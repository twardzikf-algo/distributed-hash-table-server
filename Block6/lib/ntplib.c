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
#include <sys/time.h>
#include <time.h>
#include <math.h>

#define TCP 0
#define UDP 1

/* protocol macros */
#define LI0 ( 1 << 7 )
#define LI1 ( 1 << 6 )

#define VN0 ( 1 << 5 )
#define VN1 ( 1 << 4 )
#define VN2 ( 1 << 3 )

#define MD0 ( 1 << 2 )
#define MD1 ( 1 << 1 )
#define MD2 ( 1 )

typedef struct {
    int sockfd;
    struct addrinfo* addrinfo;
    char role[7];
} Connection;

typedef struct {
   unsigned long t1_microsec;
    unsigned long t2_microsec;
    unsigned long t3_microsec;
    unsigned long t4_microsec;
    unsigned long delay_microsec;
    unsigned long offset_microsec;
} RequestTimes;

typedef struct {
    unsigned int flags;
    unsigned int root_delay;
    unsigned int root_dispersion;
    unsigned int ref_id;
    unsigned int ref_timestamp_sec;
    unsigned int ref_timestamp_frac;
    unsigned int origin_timestamp_sec;
    unsigned int origin_timestamp_frac;
    unsigned int recv_timestamp_sec;
    unsigned int recv_timestamp_frac;
    unsigned int transmit_timestamp_sec;
    unsigned int transmit_timestamp_frac;
} NetworkPacket;

typedef struct {
    char leap_indicator;
    char version;
    char mode;
    char stratum;
    char poll;
    char precision;
    unsigned int root_delay;
    unsigned int root_dispersion;
    unsigned int ref_id;
    unsigned int ref_timestamp_sec;
    unsigned int ref_timestamp_frac;
    unsigned int origin_timestamp_sec;
    unsigned int origin_timestamp_frac;
    unsigned int recv_timestamp_sec;
    unsigned int recv_timestamp_frac;
    unsigned int transmit_timestamp_sec;
    unsigned int transmit_timestamp_frac;
} HostPacket;

/***** packet function: marshal(). unmarshal(), print() *****/
void unmarshal_packet( NetworkPacket *networkPacket, HostPacket *hostPacket ) {
  
    char flags[4];
    snprintf(flags, sizeof(flags), "%d", ntohl( networkPacket->flags ) );
    
    hostPacket->leap_indicator =  ( flags[0] & ( LI0 | LI1 ) ) >> 6;
    hostPacket->version = (flags[0] & ( VN0 | VN1 | VN2 ) ) >> 3;
    hostPacket->mode = flags[0] & ( (MD0 | MD1 | MD2) );
    hostPacket->stratum = flags[1];
    hostPacket->poll = flags[2];
    hostPacket->precision = flags[3];

    hostPacket->root_delay = ntohl( networkPacket->root_delay );
    hostPacket->root_dispersion = ntohl( networkPacket->root_dispersion );
    hostPacket->ref_id = ntohl( networkPacket->ref_id );
    
    hostPacket->ref_timestamp_sec = ntohl( networkPacket->ref_timestamp_sec );
    hostPacket->ref_timestamp_frac = ntohl( networkPacket->ref_timestamp_frac );
    hostPacket->origin_timestamp_sec = ntohl( networkPacket->origin_timestamp_sec );
    hostPacket->origin_timestamp_frac = ntohl( networkPacket->origin_timestamp_frac );
    hostPacket->recv_timestamp_sec = ntohl( networkPacket->recv_timestamp_sec );
    hostPacket->recv_timestamp_frac = ntohl( networkPacket->recv_timestamp_frac );
    hostPacket->transmit_timestamp_sec = ntohl( networkPacket->transmit_timestamp_sec );
    hostPacket->transmit_timestamp_frac = ntohl( networkPacket->transmit_timestamp_frac );
}
void marshal_packet( NetworkPacket *networkPacket, HostPacket *hostPacket ) {
     
    int flags = 0;
    flags |= ( ( 1 << 29 ) | ( 1 << 25 ) | ( 1 << 24 ) );
  
    networkPacket->flags = htonl( flags );
    
    networkPacket->root_delay = htonl( hostPacket->root_delay );
    networkPacket->root_dispersion = htonl( hostPacket->root_dispersion );
    networkPacket->ref_id = htonl( hostPacket->ref_id );
    const time_t current = time( NULL );
    networkPacket->ref_timestamp_sec = htonl( current );
    networkPacket->ref_timestamp_frac = htonl( hostPacket->ref_timestamp_frac );
    networkPacket->origin_timestamp_sec = htonl( hostPacket->origin_timestamp_sec );
    networkPacket->origin_timestamp_frac = htonl( hostPacket->origin_timestamp_frac );
    networkPacket->recv_timestamp_sec = htonl( hostPacket->recv_timestamp_sec );
    networkPacket->recv_timestamp_frac = htonl( hostPacket->recv_timestamp_frac );
    networkPacket->transmit_timestamp_sec = htonl( hostPacket->transmit_timestamp_sec );
    networkPacket->transmit_timestamp_frac = htonl( hostPacket->transmit_timestamp_frac );
}
void print_packet( HostPacket *hostPacket ) {
    printf("[client]: Received Packet: \n");
    printf("\t  LI: %hu; VN: %hu; Mode: %hu;\n", hostPacket->leap_indicator, hostPacket->version, hostPacket->mode );
    printf("\t  Stratum: %hu; Poll: %hu; Precision: %hu;\n", hostPacket->stratum, hostPacket->poll, hostPacket->precision );
    printf("\t  Root Delay: %u\n", hostPacket->root_delay );
    printf("\t  Root Dispersion: %u\n", hostPacket->root_dispersion );
    printf("\t  Reference ID: %u\n", hostPacket->ref_id );
    printf("\t  Reference Timestamp: sec: %u frac: %u\n", hostPacket->ref_timestamp_sec, hostPacket->ref_timestamp_frac );
    printf("\t  Origin Timestamp: sec: %u frac: %u\n", hostPacket->origin_timestamp_sec, hostPacket->origin_timestamp_frac );
    printf("\t  Receive Timestamp: sec: %u frac: %u\n", hostPacket->recv_timestamp_sec, hostPacket->recv_timestamp_frac );
    printf("\t  Transmit Timestamp: sec: %u frac: %u\n", hostPacket->transmit_timestamp_sec, hostPacket->transmit_timestamp_frac );
    
    
}

/***** time functions: *****/
void set_t1( RequestTimes *times ) { 
    struct timeval tv;
    gettimeofday(&tv,NULL);
    times->t1_microsec = 1000000 * tv.tv_sec + tv.tv_usec;
    
}
void set_t4( RequestTimes *times ) { 
    struct timeval tv;
    gettimeofday(&tv,NULL);
    times->t4_microsec = 1000000 * tv.tv_sec + tv.tv_usec;
    
}
void set_t2( RequestTimes *times , HostPacket *hostPacket) { 
    times->t2_microsec = ( hostPacket->recv_timestamp_sec-2208988800 )*1000000 + (int)(hostPacket->recv_timestamp_frac/ pow(2, 32)*1000000);
}
void set_t3( RequestTimes *times , HostPacket *hostPacket) { 
    times->t3_microsec = ( hostPacket->transmit_timestamp_sec-2208988800 )*1000000 + (int) (hostPacket->transmit_timestamp_frac/ pow(2, 32)*1000000);
}
void set_delay( RequestTimes *times ) {
    times->delay_microsec = (times->t4_microsec - times->t1_microsec) - (times->t3_microsec - times->t2_microsec);
}
void set_offset( RequestTimes *times ) {
    times->offset_microsec = ( ( times->t2_microsec - times->t1_microsec ) + (times->t3_microsec - times->t4_microsec) ) / 2;
}
void print_times( RequestTimes *times ) {
    
    printf("\t  -------------------------\n");
    printf("\t  t1: %ld\n", times->t1_microsec );
    printf("\t  t2: %ld\n", times->t2_microsec );
    printf("\t  t3: %ld\n", times->t3_microsec );
    printf("\t  t4: %ld\n", times->t4_microsec );
    printf("\t  offset: %ld\n", times->offset_microsec );
    printf("\t  delay: %ld\n", times->delay_microsec );
    printf("\t  -------------------------\n");
}
/***** connection functions: setup(), close(), send(), recv() *****/
Connection * connection_setup(int protocol, char *address, char *port, int isServer) {
    int sockfd=0; 
    struct addrinfo hints; 
    struct addrinfo *results; 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = (protocol == UDP)?SOCK_DGRAM:SOCK_STREAM; 
    
    int retval = 0;
    if((retval = getaddrinfo(address, port, &hints, &results))!=0) {
        perror("Could not find address!");
        freeaddrinfo(results);
        exit(1);
    }
  
    while(results!=NULL) {
        if((sockfd = socket(results->ai_family,results->ai_socktype,results->ai_protocol))==-1) {
            results = results->ai_next;
            continue;
        }
        if(isServer) {
            
            if(bind(sockfd, results->ai_addr, results->ai_addrlen)==-1) {
                results = results->ai_next;
                continue;
            }
        }
        break;
    }

    // make the use of open sockets possible
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if(results==NULL) return NULL;
    Connection *connection = calloc(1, sizeof(Connection));
    connection->sockfd = sockfd;
    connection->addrinfo = results;
    if(isServer) strncpy(connection->role,"server",6);
    else strncpy(connection->role,"client",6);
    connection->role[6]='\0';
    
    if( connection == NULL ) {
        perror( "[client]: Connection setup failed " );
        return NULL;
    }       
    
    if( connect( connection->sockfd, connection->addrinfo->ai_addr,connection->addrinfo->ai_addrlen )==-1 ) {
        perror("[client]: connect() ");
        return NULL;
    }
    
    char s[INET_ADDRSTRLEN];
    inet_ntop(connection->addrinfo->ai_family, &(((struct sockaddr_in*)connection->addrinfo->ai_addr)->sin_addr), s, sizeof(s));
    printf("[client]: connected to %s\n", s);
    
    return connection;
}

int connection_close(Connection *connection) {
    
    if( close(connection->sockfd) !=0 ) {
        fprintf(stderr,"[%s]: connection closing error.\n",connection->role);
        return 1;
    }
    if ( connection->addrinfo != NULL )
    {
        freeaddrinfo(connection->addrinfo);
    }
    free( connection );
    return 0;
}

size_t connection_send_udp(Connection *connection, char *buffer) {
    size_t bytes_sent = 0;
    if(( bytes_sent = sendto(connection->sockfd, buffer, 48, 0, connection->addrinfo->ai_addr, connection->addrinfo->ai_addrlen) ) == -1) {
        perror("[error]: Could not send data ");
        return -1;
    }
    //printf("[%s]: data sent\n",connection->role);
    return bytes_sent;
}
int connection_recv_udp(Connection *connection, char *buffer) {
    ssize_t bytes_received = 0;
    if ( (bytes_received = recvfrom(connection->sockfd, buffer, 48, 0, connection->addrinfo->ai_addr, &connection->addrinfo->ai_addrlen) ) < 1)
    {
        fprintf(stderr,"[%s]: could not receive data!\n",connection->role);
        return -1;
    }
    return bytes_received;
}
