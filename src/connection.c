#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

//structure containing data to be send to the given thread
struct thread_data_t {
    int connection_socket_descriptor;
};

//finding full path to a html file
char* filePath(char* page) {
    char *file = malloc(strlen("resources") + strlen(page) + 1); // +1 for the null-terminator
    strcpy(file, "resources");
    strcat(file, page);
    return file;
}

//sending http response to the client
void sendResponse(int thread_desc, int status, char* message, int length) {
    char buf[100];
    snprintf(buf, 100, "HTTP/1.1 %d %s\r\n", status, message);
    int err = write(thread_desc, buf, sizeof(char)*strlen(buf));
    if (err < 0) {
        printf("Błąd przy wysyłaniu odpowiedzi, kod błędu: %d\n", err);
        exit(-1);
    }
    if (length >= 0) { // negative number means we don't want to send length
        snprintf(buf, 100, "Content-Length: %d\r\n", length);
        err = write(thread_desc, buf, sizeof(char)*strlen(buf));
        if (err < 0) {
            printf("Błąd przy wysyłaniu odpowiedzi, kod błędu: %d\n", err);
            exit(-1);
        }
    }
    err = write(thread_desc, "Content-Type: text/html\r\n\r\n", 27);
    if (err < 0) {
        printf("Błąd przy wysyłaniu odpowiedzi, kod błędu: %d\n", err);
        exit(-1);
    }
}

void *ThreadBehavior(void *t_data) {
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t *) t_data;
    char request_type[10];
    char protocol_type[10]; //length of HTTP/1.1\r\n which is expected
    char page[50]; //random buffer - length of monstera.html is 13 but sth longer may be put
    int thread_desc = th_data->connection_socket_descriptor;
    char request_buffer[300];
    int i = 0;
    char buf[1];
    int error;

    while ((error = read(thread_desc, buf, 1)) > 0) {
        if (buf[0] == '\n') {
            break;
        }
        request_buffer[i] = buf[0];
        i++;
    }
    if (error == 0) {
        printf("Nastąpiło nieoczekiwane rozłączenie z klientem\n");
        exit(-1);
    } else if (error < 0) {
        sendResponse(thread_desc, 500, "Internal Server Error", -1);
    }

    if ((sscanf(request_buffer, "%s %s %s", request_type, page, protocol_type) == 3)) {
        if (strcmp(protocol_type, "HTTP/1.1") != 0) {
            sendResponse(thread_desc, 505, "HTTP Version Not Supported", -1);
            free(th_data);
            close(thread_desc);
            pthread_exit(NULL);
        }
        char *file = filePath(page);

        if (strcmp(request_type, "GET") == 0) {
            FILE *requested_file;
            requested_file = fopen(file, "r");
            if (requested_file) {
                //find the size of the file and then get back to the beginning
                fseek(requested_file, 0L, SEEK_END);
                int file_size = ftell(requested_file);
                fseek(requested_file, 0L, SEEK_SET);
                char buffer[file_size];
                for (int i = 0; i < file_size; i++) {
                    fscanf(requested_file, "%c", &buffer[i]);
                }
                sendResponse(thread_desc, 200, "OK", file_size);
                int err = write(thread_desc, buffer, file_size); //send response body
                if (err < 0) {
                    printf("Błąd przy wysyłaniu odpowiedzi, kod błędu: %d\n", err);
                    exit(-1);
                }
                fclose(requested_file);
            } else {
                sendResponse(thread_desc, 404, "Not Found", -1);
            }
        } else if (strcmp(request_type, "HEAD") == 0) {
            FILE *requested_file;
            requested_file = fopen(file, "r");
            if (requested_file) {
                //find the size of the file and then get back to the beginning
                fseek(requested_file, 0L, SEEK_END);
                int file_size = ftell(requested_file);
                fseek(requested_file, 0L, SEEK_SET);
                sendResponse(thread_desc, 200, "OK", file_size);
                fclose(requested_file);
            } else {
                sendResponse(thread_desc, 404, "Not Found", -1);
            }
        } else if (strcmp(request_type, "PUT") == 0) {
            short file_exists = 0; //check if file exists for appropriate response
            if (access(file, F_OK) == 0) {
                file_exists = 1;
            }

            short gotContentLength = 0;
            char content[16] = "Content-Length: ";
            char ending[4] = "\r\n\r\n";
            int matched_content = 0;
            int n_count = 0;
            char length[10];
            while ((error = read(thread_desc, buf, 1)) > 0) {
                if (buf[0] == content[matched_content]) {
                    matched_content++;
                } else {
                    matched_content = 0;
                }
                if (matched_content == 16) {
                    matched_content = 0;
                    while ((error = read(thread_desc, buf, 1)) > 0) {
                        if (buf[0] == '\n') {
                            break;
                        }
                        length[matched_content] = buf[0];
                        matched_content++;
                    }
                    gotContentLength = 1;
                    //reading the length until the \n is found
                    matched_content = 0;
                }
                if (buf[0] == ending[n_count]) {
                    n_count++;
                } else {
                    n_count = 0;
                }
                if (n_count == 4) {
                    break;
                }
            }
            if (error == 0) {
                printf("Nastąpiło nieoczekiwane rozłączenie z klientem\n");
                exit(-1);
            } else if (error < 0) {
                sendResponse(thread_desc, 500, "Internal Server Error", -1);
                free(th_data);
                close(thread_desc);
                pthread_exit(NULL);
            }
            if (gotContentLength == 0){
                sendResponse(thread_desc, 411, "Length Required", -1);
                free(th_data);
                close(thread_desc);
                pthread_exit(NULL);
            }
            int size = atoi(length);

            // create temporary file for writing
            char tmp_name[50];
            memset(tmp_name,0,sizeof(tmp_name));
            strcpy(tmp_name, "resources/tmp/XXXXXX");
            mkstemp(tmp_name);
            FILE *tmp_file;
            tmp_file = fopen(tmp_name, "w");
            if (!tmp_file) {
                sendResponse(thread_desc, 500, "Internal Server Error", -1);
                free(th_data);
                close(thread_desc);
                pthread_exit(NULL);
            }

            for (int i = 0; i < size; i++) {
                error = read(thread_desc, buf, 1);

                if (error < 0) {
                    sendResponse(thread_desc, 500, "Internal Server Error", -1);
                    free(th_data);
                    close(thread_desc);
                    pthread_exit(NULL);
                } //TODO error == 0

                fputc(buf[0], tmp_file); //TODO error with writing
            }
            error = rename(tmp_name, file);
            remove(tmp_name);
            if (error < 0) {
                if (!tmp_file) {
                    sendResponse(thread_desc, 500, "Internal Server Error", -1);
                    free(th_data);
                    close(thread_desc);
                    pthread_exit(NULL);
                }
            }
            if (error == 0) {
                printf("Nastąpiło nieoczekiwane rozłączenie z klientem\n");
                exit(-1);
            } else if (error < 0) {
                sendResponse(thread_desc, 500, "Internal Server Error", -1);
                pthread_mutex_unlock(&mutex_server);
                free(th_data);
                close(thread_desc);
                pthread_exit(NULL);
            }

            if (file_exists == 0) {
                sendResponse(thread_desc, 201, "Created", -1);
            } else {
                sendResponse(thread_desc, 204, "No Content", -1);
            }
            fclose(tmp_file);
        } else if (strcmp(request_type, "DELETE") == 0) {
            int del = remove(file);
            if (!del) {
                sendResponse(thread_desc, 204, "No Content", -1);
            } else {
                sendResponse(thread_desc, 404, "Not Found", -1);
            }
        } else {
            sendResponse(thread_desc, 501, "Not Implemented", -1);
        }
    } else {
        sendResponse(thread_desc, 400, "Bad Request", -1);
    }
    free(th_data);
    close(thread_desc);
    pthread_exit(NULL);
}

//handling connection with a new client
void handleConnection(int connection_socket_descriptor) {
    int create_result = 0;
    pthread_t thread;
    struct thread_data_t *t_data = (struct thread_data_t *) malloc(sizeof(struct thread_data_t));
    t_data->connection_socket_descriptor = connection_socket_descriptor;
    create_result = pthread_create(&thread, NULL, ThreadBehavior, (void *) t_data);
    if (create_result) {
        printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
        exit(-1);
    }
}
