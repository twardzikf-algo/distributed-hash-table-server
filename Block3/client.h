#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

#ifndef CLIENT_H_
#define CLIENT_H_

#define MAXDATASIZE 2048
#define TCP 0
#define UDP 1

#define DEL ( 1 )
#define SET ( 1 << 1 )
#define GET ( 1 << 2 )
#define ACK ( 1 << 3 )

typedef struct {
    int sockfd;
    struct addrinfo* addrinfo;

} Connection;

typedef struct {

    short set;
    short get;
    short del;
    short ack;
    short transaction_id;
    short key_length;
    short value_length;
    char* key;
    char* value;
    int size; // additional  attribute for storing the size of output buffer after marshalling

} Message;

Connection * connection_create(int protocol, char *address, char *port);

int connection_close(Connection *connection);

char * connection_recv_tcp(Connection *connection);

int connection_send_tcp(Connection *connection, char *message, int msg_size);

Message* unmarshall(char *input, Message *m);

char* marshall(Message *m);

char* get(Connection* connection, char* key);

int set(Connection* connection, char* key, char* value);

int del(Connection* connection, char* key);

#endif