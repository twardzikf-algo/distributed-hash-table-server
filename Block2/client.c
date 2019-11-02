#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>

int main(int argc, char *argv[]){

    FILE *log = fopen("log.txt", "w");//log file creation

    if (argc != 3)//check proper usage
    {
        fprintf(log, "Please use ./client ip/DNSAddress port\n");
        return -1;
    }

    struct addrinfo *server;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(argv[1], argv[2], &hints, &server);

    if(status != 0){//check internet address assigning
        fprintf(log, "Wrong domain or port, error %d\n", status);
        return -1;
    }

    int s = socket(server -> ai_family, server -> ai_socktype, server -> ai_protocol);

    if(s == -1){//check socket
        fprintf(log, "Socket creation failed\n");
    }

    if(connect(s, server -> ai_addr, server -> ai_addrlen) == -1){//check connection
        fprintf(log, "Connecting failed\n");
        close(s);
        return -1;
    }

    char buffer[1];
    int i = 0;
    int bytes_received = 0;

    while(true){
        bytes_received = recv(s, buffer, 1, 0);//try to receive 1 char
        if(bytes_received == -1){//fail case
            fprintf(log, "No characters have been received \n");
        }
        else if(bytes_received == 0 && i == 0){//empty message case
            fprintf(log, "0 characters have been received\n");
        }
        else if(bytes_received == 1){//good case
            i++;
            printf("%s", buffer);
            continue;
        }
        break;
    }
    close(s);
    return 0;
}