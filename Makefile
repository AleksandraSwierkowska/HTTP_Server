CC = gcc
CFLAGS = -Wall -pthread

server:
	$(CC) $(CFLAGS) src/main.c src/initializator.c src/connection.c -o server


