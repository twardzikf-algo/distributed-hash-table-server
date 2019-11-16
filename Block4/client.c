
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <unistd.h>
#include <arpa/inet.h>
#include "communication.h"
#define MAX_LENGTH_STRING 1024
#define LOCALHOST "loc"
static unsigned int server_port = 0;
#define LOCALHOST_IP 2130706433


static inline void ip_bytes(char *dest, int ip) 
{
    for (int i = 0; i < 4; i++) 
        *(dest + i) = (LOCALHOST_IP & (0xff << i * 8)) >> (i * 8);
}


static void build_msg(comm_nessage *msg, int flag)
{
    char tmp[MAX_LENGTH_STRING];
    printf("Give key:\n");
    scanf("%s", tmp);
    (*msg).key_length[0] = strlen(tmp) & 0xff;
    (*msg).key_length[1] = strlen(tmp) & (0xff << 8);
    (*msg).key = strdup(tmp);
    if (flag) {
        printf("Give value:\n");
        scanf("%s", tmp);
        (*msg).value_length[0] = strlen(tmp) & 0xff;
        (*msg).value_length[1] = strlen(tmp) & (0xff << 8);
        (*msg).value = strdup(tmp);
    }
}
void client_interaction(){
	int clientSocket;
	struct sockaddr_in serverAddr;
	char buffer[1024];
    printf("Choose the option:\n\t 1) get\n\t2)set\n\t3)delete\n");
    
    char c;
    while (c = getchar()) {
        clientSocket = socket(PF_INET, SOCK_STREAM, 0);
        printf("[+]Client Socket Created Sucessfully.\n");

        memset(&serverAddr, '\0', sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(server_port);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        printf("[+]Connected to Server.\n");

        printf("ret %c\n", c);
        comm_nessage msg = { .function = 0, .id[0] = 100, .key = NULL, .value = NULL}; 
        ip_bytes(msg.ip, LOCALHOST_IP);
        sscanf(&msg.port[0], "%d", server_port & 0xff);
        sscanf(&msg.port[1], "%d", server_port & (0xff << 8));
        
        switch (c) {
            char tmp[MAX_LENGTH_STRING];
            case '1':
                SET_FLAGS(msg.function, GET);
                build_msg(&msg, 0);
                break;
            case '2':
                SET_FLAGS(msg.function, SET);
                build_msg(&msg, 1);
                break;
            case '3':
                SET_FLAGS(msg.function, DEL);
                build_msg(&msg, 0);
                break;
            default:
                break;
        }
        send(clientSocket, &msg, sizeof(comm_nessage), 0);
        if (msg.key != NULL) {
            send(clientSocket, msg.key, strlen(msg.key), 0);
            free(msg.key);
        }
        if (msg.value != NULL) {
           send(clientSocket, msg.value, strlen(msg.value), 0); 
            free(msg.value);
        }        
    }
	
//     read(clientSocket, buffer, MAX_LENGTH_STRING - 1);

	printf("[+]Closing the connection.\n");
    close(clientSocket);
}
int main(int argc, char *argv[]){
    if (argc > 1) {
            server_port = atoi(argv[1]);
    }
    while (1)
        client_interaction();
    return 0;
}
