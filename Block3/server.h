#ifndef SERVER_H_
#define SERVER_H_

#define MAXDATASIZE 2048
#define TCP 0
#define UDP 1
#define BACKLOG 100

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
    short transaction_id;
    short key_length;
    short value_length;

} Flags;

Connection * connection_create(int protocol, char *address, char *port);

int connection_close(Connection *connection);

char * connection_recv_tcp(Connection *connection);

int connection_send_tcp(Connection *connection, char *message, int client_sockfd);

Item* unmarshall(char *input, Flags *flags);

char* marshall(char* key, char *value, Flags *flags);

#endif