#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "communication.h"

#define MAX_LENGTH_STRING 1024 //is this temporary or how do we handle larger requests

static unsigned int server_port = 0; //this here is our starting point, and we set it back to 0 inside function

int create_socket()
{
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    int listen_sock; //till here is cool
    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("could not create listen socket\n");
        return -1;
    } //creation of socket for listening
    if ((bind(listen_sock, (struct sockaddr *)&server_address,
                sizeof(server_address))) < 0) {
        printf("could not bind socket\n");
        return -2;
    } //bind
    socklen_t len = sizeof(server_address);
    if (getsockname(listen_sock, (struct sockaddr *)&server_address, &len) == -1) {
            perror("getsockname");
            return -3;
    } //te -1, -2, -3 co Wissem zrobił jak taka rewelacja "czemu ja na to nie wpadłem mnie siekło"
    printf("localhost:%d\n", ntohs(server_address.sin_port));
    int wait_size = 16;  
    if (listen(listen_sock, wait_size) < 0) {
        printf("could not open socket for listening\n");
        return -4;
    }  //this socket is for attaching the next peer, right?
    return listen_sock;
}

int create_socket_port(int port) 
{
    server_port = port;
    int ret = create_socket();
    server_port = 0; //how proper is this management
    return ret; //so this creates a cosket and returns the port adress to place info at?
}
int client_interaction(int sock)
{
    int n = 0;
    char buffer[MAX_LENGTH_STRING];
    char *pbuffer = buffer;
    while ((n = recv(sock, pbuffer, MAX_LENGTH_STRING, 0)) > 0) {
        printf("received: '%s'\n", buffer);
        n += strlen(buffer);
        send(sock, buffer, strlen(buffer), 0);
    }
    return n;
}



void parse_msg(comm_nessage msg) //unmarshalling ? space technology
{
    if (READ_FLAGS(msg.function, GET))
        printf("Call Get\n");
    else if (READ_FLAGS(msg.function, SET))
        printf("Call SET\n");
    else if (READ_FLAGS(msg.function, DEL))
        printf("Call DEL\n");
}
int wait_for_connection(int listen_sock)
{
	struct sockaddr_in client_address;
	unsigned int client_address_len = 0;
	char buf[BUFSIZ];
    
	while (1) {
		// socket to transmit data over network
		int sock;
		if ((sock =
		         accept(listen_sock, (struct sockaddr *)&client_address,
		                &client_address_len)) < 0) {
			printf("could not open a socket to accept data\n");
			return 1;
		}
// 		get_ip_info(inet_ntoa(client_address.sin_addr));
		printf("client connected with ip address: %s\n",
		       inet_ntoa(client_address.sin_addr));
		if (!fork()) { //we got advice not to involve fork, how does this work?
            comm_nessage msg;
			memset(buf, 0, MAX_LENGTH_STRING);
			read(sock, &msg, sizeof(comm_nessage));
            read(sock, buf, two_byte_int(msg.key_length));
            printf("Receiving: %s\nWrite to send", buf);
            parse_msg(msg);
        }
        close(sock);     
    }
    return 0;
}

int main(){
    int listen_sock = create_socket();//we get the rough idea
    wait_for_connection(listen_sock);
    close(listen_sock);
    return 0;
}
