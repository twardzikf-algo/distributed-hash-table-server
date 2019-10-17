CC = gcc
CFLAGS_D = -Wall -g

all: client server

client: client.c
	$(CC) -o client client.c
server: server.c
	$(CC) -o server server.c

debug:
	$(CC) $(CFLAGS_D) -o client client.c
	$(CC) $(CFLAGS_D) -o server server.c

clean:
	rm client server
