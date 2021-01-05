void handleConnection(int connection_socket_descriptor);
void *ThreadBehavior(void *t_data);
char* filePath(char* page);
void sendResponse(int thread_desc, int status, char* message, int length);
