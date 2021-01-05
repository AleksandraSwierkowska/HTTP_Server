#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "initializator.h"
#include "connection.h"
#define DEFAULT_SERVER_PORT 1234

int main(int argc, char* argv[]) {
    int server_socket_descriptor;
    int connection_socket_descriptor;
    int server_port;
    //initialization of the server on the default port or the chosen port
    if (argc == 1) {
        server_port = DEFAULT_SERVER_PORT;
    } else {
        server_port = atoi(argv[1]);
    }
   server_socket_descriptor = initializeServer(server_port);
   while(1) {
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0) {
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }

       handleConnection(connection_socket_descriptor);
   }

   close(server_socket_descriptor);
   return(0);
}
