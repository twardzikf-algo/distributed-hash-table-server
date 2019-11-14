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

#include "item.h"
#include "server.h"



/* *****
 * For this homework we took a ready implementation
 *  of a simple hashtable from https://gist.github.com/phsym/4605704
 * *****/
#include "hashtable.c"


int main(int argc, char *argv[]) {

    if(argc!=2) {
        fprintf(stderr,"wrong usage: ./server <port_number> \n");
        return 1;
    }

    Connection *connection = connection_create(TCP, NULL, argv[1]);
    if(connection == NULL) {
        perror("[server]: socket() ");
        return 1;
    }

    if(listen(connection->sockfd, BACKLOG)==-1) {
        perror("[server]: listen() ");
        return 1;
    }
    printf("[server]: waiting for connections... \n");

    /* setup the hashtable for items */
    hashtable_t *database = ht_create(200);

    socklen_t sin_size; // length of client_addr will be stored here
    struct sockaddr_in client_addr; //  address information of a connecting client
    sin_size = sizeof( client_addr );

    while(1) {
        int client_sockfd = 0;
        fflush(stdout);

        if((client_sockfd = accept(connection->sockfd, (struct sockaddr*)&client_addr, &sin_size)) == -1) {
            perror("[server]: accept() ");
            break;
        }

        /* create a connection struct for the client connection
         * beacuse the functions for receiving and sending expect a Connectio nstruct a sthe parameter,
         * to standardize things a bit and make universla for future potential use in next homework */

        Connection* client_connection = calloc(1, sizeof(Connection));
        client_connection->sockfd = client_sockfd;
        client_connection->addrinfo = NULL;

        char s[INET_ADDRSTRLEN];
        inet_ntop(client_addr.sin_family, &(client_addr.sin_addr), s, sizeof(s));
        printf("[server]: Connected to : %s\n", s);


        char *buffer = connection_recv_tcp(client_connection);
        Flags *flags = malloc(sizeof(Flags));

        if(buffer!=NULL) {

            Item *input = unmarshall(buffer, flags);
            char* answer = NULL;

            if (flags->set == 1) {
                ht_put(database, input->key, input->value);
                printf("[server]: SET Item(key: %s,value: %s) to the hashtable\n", input->key, input->value);
            }
            else if (flags->get == 1) {
                answer = ht_get(database, input->key);
                if(answer==NULL) flags->get = 0;
                else printf("[server]: GET Item(key: %s,value: %s) from the hashtable\n", input->key, answer);
            }
            else if (flags->del == 1) {
                if(ht_get(database, input->key) == NULL) flags->del = 0;
                else {
                    printf("[server]: DEL Item(key: %s,value: %s) from the hashtable\n", input->key, (char*) ht_get(database, input->key));
                    ht_remove(database, input->key);
                }
            }

            char *output = marshall(input->key, answer, flags);
            if( connection_send_tcp(client_connection, output, client_sockfd)==1) fprintf(stderr,"[server]: sending error.\n");
            free(output);
        }
        /* cleaning after the request */
        connection_close(client_connection);
        free(flags);

    }

    /* final cleanign after the server is done */
    ht_destroy(database);

    if(connection_close(connection) == 1) {
        fprintf(stderr,"[server]: close() error on client connection\n");
        return 1;
    }
    printf("[server]: closing...\n");
    return 0;
}
