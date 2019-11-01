#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>

int main(int argc, char *argv[]){

    char str[500];
    FILE *fptr = fopen("errorFile.txt", "w");

    if (argc != 3)
    {
        fprintf(fptr, "Error! Valid input: ./client <ip_address> <port>\n", str );
        return -1;
    }

    struct addrinfo *server;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(argv[1], argv[2], &hints, &server);

    if(status != 0){
        fprintf(fptr, "Socket creation failed, the status is: %d\n", str, status);
        return -1;
    }

    int s = socket(server -> ai_family, server -> ai_socktype, server -> ai_protocol);

    if(s == -1){
        fprintf(fptr, "Socket binding failed \n", str);
    }

    if(connect(s, server -> ai_addr, server -> ai_addrlen) == -1){
        fprintf(fptr, "Connection creation failed \n", str);
        close(s);
        return -1;
    }

    char buffer[1];
    int i = 0;
    int bytes_received = 0;

    while(true){
        bytes_received = recv(s, buffer, 1, 0);
        if(bytes_received == -1){
            fprintf(fptr, "No characters have been received \n", str);
        }
        else if(bytes_received == 0 && i == 0){
            fprintf(fptr, "0 characters have been received\n", str);
        }
        else if(bytes_received == 1){
            i++;
            fprintf("%s", buffer);
            continue;
        }
        break;
    }

    close(s);

    return 0;
}