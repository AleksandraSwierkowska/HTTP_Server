#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define SERVER_PORT 1234
#define QUEUE_SIZE 5 //fixed size - safe for the computer resources

#define HERE() printf("I'm in %s @ %d\n", __func__, __LINE__)


//mutexes for all the html files TODO : PUT will be problematic here
pthread_mutex_t mutex_main = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_maple = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_monstera = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_orchid = PTHREAD_MUTEX_INITIALIZER;

//structure containing data to be send to the given thread
struct thread_data_t
{
	int connection_socket_descriptor;
};

void *ThreadBehavior(void *t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    int error;
    char request_type[6]; //length of "delete"
    char protocol_type[8]; //length of HTTP/1.1 which is expected TODO : \r\n may occur
    char page[50]; //random buffer - length of monstera.html is 13 but sth longer may be put
    int thread_desc = th_data->connection_socket_descriptor;
    char request_buffer[300];
    char buf[1];
    int i = 0;
    while (read(thread_desc, buf, 1) > 0){
        if (buf[0] == '\n') {
            break;
        }
        request_buffer[i] = buf[0];
        i++;
        //TODO error
    }
    if ((sscanf(request_buffer, "%s %s", request_type, page) == 2)){
        //TODO: add strcmp(protocol_type, "HTTP/1.1") when it works
        if (strcmp(request_type, "GET") == 0){
            //TODO : check if mutex' up
            char *file = malloc(strlen("../resources") + strlen(page) + 1); // +1 for the null-terminator
            strcpy(file, "../resources");
            strcat(file, page);
            FILE *requested_file;
            requested_file = fopen(file, "r"); //TODO : r because GET - different for put/delete
            if (requested_file) {
                //find the size of the file and then get back to the beginning
                fseek(requested_file, 0L, SEEK_END);
                int file_size = ftell(requested_file);
                fseek(requested_file, 0L, SEEK_SET);
                char buffer[file_size]; //TODO : putting the file here - scanf?

                for (int i = 0; i < file_size; i++){
                    fscanf(requested_file, "%c", &buffer[i]);
                }
                char buf[100];
                write(thread_desc, "HTTP/1.1 200 OK\r\n", 17);
                snprintf(buf, 100, "Content-length: %d\r\n", file_size);
                write(thread_desc, buf, sizeof(char)*strlen(buf));
                write(thread_desc, "Content-type: text/html\r\n", 25);
                write(thread_desc, "\r\n", 2);
                write(thread_desc, buffer, file_size);
                fclose(requested_file);
            } else {
                write(thread_desc, "HTTP/1.1 404 Not Found\r\n", 24);
            }
        }
        else if (strcmp(request_type, "HEAD") == 0){
            char *file = malloc(strlen("../resources") + strlen(page) + 1); // +1 for the null-terminator
            strcpy(file, "../resources");
            strcat(file, page);
            FILE *requested_file;
            requested_file = fopen(file, "r"); //TODO : r because GET - different for put/delete
            if (requested_file) {
                //find the size of the file and then get back to the beginning
                fseek(requested_file, 0L, SEEK_END);
                int file_size = ftell(requested_file);
                fseek(requested_file, 0L, SEEK_SET);
                char buf[100];
                write(thread_desc, "HTTP/1.1 200 OK\r\n", 17);
                snprintf(buf, 100, "Content-length: %d\r\n", file_size);
                write(thread_desc, buf, sizeof(char)*strlen(buf));
                write(thread_desc, "Content-type: text/html\r\n", 25);
                write(thread_desc, "\r\n", 2);
                fclose(requested_file);
            } else {
                write(thread_desc, "HTTP/1.1 404 Not Found\r\n", 24);
            }
        }
        else if (strcmp(request_type, "PUT") == 0) {
            write(thread_desc, "HTTP/1.1 202 Accepted\r\n", 23);
            char *file = malloc(strlen("../resources") + strlen(page) + 1); // +1 for the null-terminator
            strcpy(file, "../resources");
            strcat(file, page);
            short file_exists = 0;
            if( access( file, F_OK ) == 0 ) {
                file_exists = 1;
            }
            FILE *requested_file;
            requested_file = fopen(file, "w");
            if (!requested_file) {
                //TODO error
            }
            char content_length[15] = "ontent-length: ";
            int i = 0;
            char length[10];
            while (read(thread_desc, buf, 1) > 0){
                printf("%c", buf[0]);
                if (buf[0] == content_length[i]) {
                    i++;
                } else {
                    i = 0;
                }
                if (i == 15) {
                    HERE();
                    i = 0;
                    while (read(thread_desc, buf, 1) > 0){
                        if (buf[0] == '\n') {
                            break;
                        }
                        length[i] = buf[0];
                        i++;
                    }
                    //procedura czytania liczby do \n
                    i = 0;
                }
                if (buf[0] == '\n') {
                    read(thread_desc, buf, 1);
                    if (buf[0] == '\r') {
                        read(thread_desc, buf, 1);
                        break;
                    } else {
                        continue;
                    }
                }
            } //TODO error
            HERE();
            printf("%s\n", length);
            int size = atoi(length);
            for (int i = 0 ; i < size ; i++) {
                read(thread_desc, buf, 1);
                fputc(buf[0], requested_file);
            }
            HERE();
            char buf[100];
            if (file_exists == 0) {
                write(thread_desc, "HTTP/1.1 201 Created\r\n", 22);
            } else {
                HERE();
                write(thread_desc, "HTTP/1.1 204 No Content\r\n", 25);
                //snprintf(buf, 100, "Content-Location: %s\r\n", page);
                //printf("%s\n", buf);
                //write(thread_desc, buf, sizeof(char) * strlen(buf));
            }
            HERE();
            fclose(requested_file);
        }
        else if (strcmp(request_type, "DELETE") == 0) {

        }
        else {
            //TODO wrong request
        }
    }
    close(thread_desc);
    pthread_exit(NULL);
}

//handling connection with a new client
void handleConnection(int connection_socket_descriptor) {
    int create_result = 0;
    pthread_t thread1;
    struct thread_data_t *t_data = (struct thread_data_t *) malloc(sizeof(struct thread_data_t)); //TODO : maybe put fixed size here?
    t_data->connection_socket_descriptor = connection_socket_descriptor;
    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }
}

int main(int argc, char* argv[])
{
   int server_socket_descriptor;
   int connection_socket_descriptor;
   int bind_result;
   int listen_result;
   char reuse_addr_val = 1;
   struct sockaddr_in server_address;

   //initialization of the server socket
   
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY);
   server_address.sin_port = htons(SERVER_PORT);
   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   
if (server_socket_descriptor < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
   
   setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   
   if (bind_result < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }

   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

   while(1)
   {
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0)
       {
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }

       handleConnection(connection_socket_descriptor);
   }

   close(server_socket_descriptor);
   return(0);
}
