#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#define bufferLength 512
#define BACKLOG 5

bool printQuote(char *content, char *source){

    FILE *log = fopen("log.txt", "w");
    FILE *quotes = fopen("quotes.txt", "r");
    if(quotes == NULL){
        fprintf(log, "Log file failed to open\n");
        return false;
    }

    char textBuffer[bufferLength];
    int numberOfLines = 0;
    while(!feof(quotes)){
        fgets(textBuffer, bufferLength, quotes);//fgets() reads in at most one less than size characters from stream and stores them into the buffer pointed to by s. Reading stops after an EOF or a newline. If a newline is read, it is stored into the buffer. A terminating null byte (aq\0aq) is stored after the last character in the buffer.
        numberOfLines++;
    }
    if(numberOfLines == 0 || numberOfLines == 1){
        fprintf(log, "quotes file is empty or contains only 1 line\n");
        fclose(quotes);
        return false;
    }

    srand((unsigned) time(NULL));
    int RNG = rand() % numberOfLines;
    fseek(quotes, 0, SEEK_SET);//man says SEEK_SET is example

    for(int i = 0; !feof(quotes) && i <= RNG; i++){
        fgets(textBuffer, bufferLength, quotes);
        if(strcmp(textBuffer, "\n") == 0) break;
        strcpy(content, textBuffer);
    }
    fclose(quotes);
    return true;
}

int main(int argc, char *argv[]) {

    FILE *log = fopen("log.txt", "w");

    if(argc != 3){
        fprintf(log, "use ./server ip/DNSAddress file \n");
        return -1;
    }

    struct addrinfo *server;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, argv[1], &hints, &server);
    if(status != 0){
        fprintf(log, "getaddrinfo failed and says: %d\n", status);
        return -1;
    }

    int s = socket(server -> ai_family, server -> ai_socktype, server -> ai_protocol);
    if(s == -1){
        fprintf(log, "socket creation failed \n");
        return -1;
    }

    if(bind(s, server -> ai_addr, server -> ai_addrlen) == -1){
        fprintf(log, "binding of socket failed \n");
        close(s);
        return -1;
    }

    if(listen(s, BACKLOG) == -1){
        fprintf(log, "listen failed\n");
        close(s);
        return -1;
    }

    int s_client;
    struct sockaddr_storage address_client;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    char content[bufferLength];

    if(!printQuote(content, argv[2])){
        close(s);
        return -1;
    }

    printf("server ready\n");

    while(true){
        s_client = accept(s, (struct sockaddr *)&address_client, &sin_size);
        if(s_client == -1){
            fprintf(log, "accepting connection failed\n");
            continue;
        }
        printQuote(content, argv[2]);
        send(s_client, &content, strlen(content), 0);
        fprintf(log, "server sent: %s\n", content);
        close(s_client);
    }

    close(s);
    return 0;
}