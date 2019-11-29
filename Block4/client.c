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
#define LOCALHOST_IP 2130706433

static inline void ip_bytes(char *dest, int ip) 
{
    for (int i = 0; i < 4; i++) 
        *(dest + i) = (LOCALHOST_IP & (0xff << i * 8)) >> (i * 8);
}

int validate_args(int argc, char *argv[]) {
    if (argc < 5 || argc > 6) {
        printf("Wrong usage: ./client <IP> <PORT> <METHOD> <KEY> [VALUE]");
        return 1;
    }
    if ((strncmp(argv[3], "SET", 3) == 0) && argc == 5) {
        printf("Wrong usage: SET method expects a value.");
        return 1;
    }
    if ((strncmp(argv[3], "GET", 3) == 0 || strncmp(argv[3], "DEL", 3) == 0) && argc == 6) {
        printf("Wrong usage: GET and DEL method don't expect any value.");
        return 1;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    printf("- client started.\n");
    if(validate_args(argc, argv) == 1) return 1;

    struct sockaddr_in server_address = { .sin_family = AF_INET, .sin_port = htons(atoi(argv[2])), .sin_addr.s_addr = inet_addr(argv[1]) };
    int client_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    rpc_msg reset_msg = {0};
    rpc_msg msg = reset_msg;
    unsigned char msg_flags = (char) 0;
    msg.key_length = strlen(argv[4]);
    msg.key = strdup(argv[4]);
    if(strncmp(argv[3], "LOOKUP", 5) == 0) {
        SET_FLAG(msg_flags, LOOKUP);
    }
    if(strncmp(argv[3], "REPLY", 5) == 0) {
        SET_FLAG(msg_flags, REPLY);
    }
    if(strncmp(argv[3], "GET", 3) == 0) SET_FLAG(msg_flags, GET);
    if(strncmp(argv[3], "DEL", 3) == 0) SET_FLAG(msg_flags, DEL);
    if(strncmp(argv[3], "SET", 3) == 0) {
        SET_FLAG(msg_flags, SET);
        msg.value_length = strlen(argv[5]);
        msg.value = strdup(argv[5]);
    }

    log_rpc_msg("me", msg, msg_flags);

    send(client_socket, &msg_flags, 1, 0);
    send(client_socket, &msg, 6, 0);
    send(client_socket, msg.key, strlen(msg.key), 0);
    free(msg.key);
    if (msg.value != NULL) {
        send(client_socket, msg.value, strlen(msg.value), 0);
        free(msg.value);
    }
    msg = reset_msg;
    read(client_socket, &msg_flags, 1);
    read(client_socket, &msg, 6);
    printf("- server responded.\n");
    log_rpc_msg(inet_ntoa(server_address.sin_addr), msg, msg_flags);

    printf("- closing the connection.\n");
    close(client_socket);
    printf("- client finished.\n");
}
