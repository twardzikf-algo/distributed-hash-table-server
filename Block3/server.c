#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "item.h"
#include "server.h"

Connection * connection_create(int protocol, char *address, char *port) {

    int sockfd=0;
    struct addrinfo hints;
    struct addrinfo *results;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = (protocol == UDP)?SOCK_DGRAM:SOCK_STREAM;

    int retval = 0;
    if((retval = getaddrinfo(address, port, &hints, &results))!=0) {
        fprintf(stderr, "[server]: Could not find address! ");
        freeaddrinfo(results);
        exit(1);
    }

    while(results!=NULL) {
        if((sockfd = socket(results->ai_family,results->ai_socktype,results->ai_protocol))==-1) {
            results = results->ai_next;
            continue;
        }
        if(bind(sockfd, results->ai_addr, results->ai_addrlen)==-1) {
            results = results->ai_next;
            continue;
        }
        break;
    }
    if(results==NULL) return NULL;

    Connection *connection = calloc(1, sizeof(Connection));
    connection->sockfd = sockfd;
    connection->addrinfo = results;
    return connection;
}

int connection_close(Connection *connection) {
    if(close(connection->sockfd)!=0) return 1;
    if (connection->addrinfo != NULL) freeaddrinfo(connection->addrinfo);
    free(connection);
    return 0;
}

char * connection_recv_tcp(Connection *connection) {

    FILE *log = fopen("log.txt", "w");
    char *buffer = calloc(MAXDATASIZE, sizeof(char) );
    ssize_t bytes_received = 0;
    if ((bytes_received = recv(connection->sockfd, buffer, MAXDATASIZE-1, 0)) < 1) {
        fprintf(stderr, "[server]: Could not receive data!");
    }
    return buffer;
}

int connection_send_tcp(Connection *connection, char *buffer, int client_sockfd) {
    FILE *log = fopen("log.txt", "w");
    fprintf(stderr, "[server]: sending response\n");
    if( send(client_sockfd, buffer, MAXDATASIZE, 0)==-1 ) return 1;
    else return 0;
}


/* function responsible for unmarshalling the message */
Item* unmarshall(char *buffer, Flags *flags) {
    flags->del = ( (buffer[0] & DEL) >0)?1:0;
    flags->set = ( (buffer[0] & SET) >0)?1:0;
    flags->get = ( (buffer[0] & GET) >0)?1:0;
    flags->transaction_id = buffer[1];

    // dummy way to get the host by order - in this case just fixed assumtpion that the host has LSB > MSB  order
    unsigned int int_key_length = (buffer[2]*16)+(buffer[3]);
    unsigned int int_value_length = (buffer[4]*16)+(buffer[5]);

    flags->key_length = int_key_length;
    flags->value_length = int_value_length;

    char *key = malloc(flags->key_length*sizeof(char)+1);
    char *value = malloc(flags->value_length*sizeof(char)+1);

    memcpy(key, buffer + 6, flags->key_length);
    key[flags->key_length] = '\0';
    memcpy(value, buffer + 6 + flags->key_length, flags->value_length);
    value[flags->value_length] = '\0';

    return item_create(key, value);
}

/* function responsible for marshalling the message */
char* marshall(char* key, char *value, Flags *flags) {
    /* just to be sure that value is always a valid string here */
    if (value == NULL) {
        value = malloc(1);
        value[0] = '\0';
    }

    /* buffer for the response */
    char *buffer = NULL;
    /* copy all parts of response to the buffer piece by piece */
    if(flags->get == 1) {
        buffer = calloc(1, ( 6 + strlen(key) + strlen(value) )*sizeof(char) );

        if( flags->del == 1 ) buffer[0] |= DEL;
        if( flags->set == 1 ) buffer[0] |= SET;
        if( flags->get == 1 ) buffer[0] |= GET;
        buffer[0] |= ACK;
        buffer[1] = flags->transaction_id;

        short net_key_length = htons(strlen(key));
        short net_value_length = htons(strlen(value));
        memcpy(buffer+2, &net_key_length, 2);
        memcpy(buffer+4, &net_value_length, 2);
        memcpy(buffer+6, key, strlen(key));
        memcpy(buffer+6+strlen(key), value, strlen(value));
    }
    else {
        buffer = calloc(1, ( 6 + 2 ));

        if( flags->del == 1 ) buffer[0] |= DEL;
        if( flags->set == 1 ) buffer[0] |= SET;
        if( flags->get == 1 ) buffer[0] |= GET;
        buffer[0] |= ACK;
        buffer[1] = flags->transaction_id;

        char *zero_key = "\0";
        char *zero_value = "\0";
        short net_key_length = 0;
        short net_value_length = 0;
        memcpy(buffer+2, &net_key_length, 2);
        memcpy(buffer+4, &net_value_length, 2);
        memcpy(buffer+6, zero_key, 1);
        memcpy(buffer+6+1, zero_value, 1);
    }

    return buffer;
}