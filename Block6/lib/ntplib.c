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

/** bit patterns for flags **/
#define LI0 ( 1 << 7 )
#define LI1 ( 1 << 6 )
#define VN0 ( 1 << 5 )
#define VN1 ( 1 << 4 )
#define VN2 ( 1 << 3 )
#define MD0 ( 1 << 2 )
#define MD1 ( 1 << 1 )
#define MD2 ( 1 )


/** dirty notes from the exercize pdf, hints and from lecture content **/
// when querying as the client: mode=3 , NTP Version is 4  (VN)
// NTP servers accesible using UDP on port 123
// answer from the server contains both timestamps (receive timestamp and transmit timestamp)

// in case of NTP #secs from 1.01.1900 and on UNix its #sec from 1.1.1970
// so the offset is 2208988800
// if you are converting from NTP to Unix you substract
// if you are converting from UNix to NTP - you add
// the format of NTP timestamp:
// - 64 bits are for the fixpointnumber
// - 32 bits for seconds and 32bits for Nachkommateil

/** structs for storing needed information in a structured matter **/

typedef struct {
    int sockfd;
    struct addrinfo* addrinfo;
} Connection;

typedef struct {
   unsigned long t1_nsec;
    unsigned long t2_nsec;
    unsigned long t3_nsec;
    unsigned long t4_nsec;
    double delay_sec;
    double offset_sec;
} Times;

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
} NetworkMessage;

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
} HostMessage;

/** functions for handling connection and sending/receiving data **/

Connection * setupConnection(char *address, char *port) {
    struct addrinfo hints, *results;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if(getaddrinfo(address, port, &hints, &results) != 0) {
        freeaddrinfo(results);
        exit(1);
    }
    int sockfd=0;
    while(results!=NULL) {
        if((sockfd = socket(results->ai_family,results->ai_socktype,results->ai_protocol))==-1) {
            results = results->ai_next;
            continue;
        }
        break;
    }

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if(results==NULL) return NULL;
    Connection *connection = calloc(1, sizeof(Connection));
    connection->sockfd = sockfd;
    connection->addrinfo = results;

    if( connect( connection->sockfd, connection->addrinfo->ai_addr,connection->addrinfo->ai_addrlen )==-1 ) {
        perror("connect() ");
        return NULL;
    }
    return connection;
}
void closeConnection(Connection *connection) {
    close(connection->sockfd);
    if(connection->addrinfo != NULL) freeaddrinfo(connection->addrinfo);
    free(connection);
}

void unmarshalMessage( NetworkMessage *networkMessage, HostMessage *hostMessage ) {
    char flags[4];
    snprintf(flags, sizeof(flags), "%d", ntohl( networkMessage->flags ) );
    hostMessage->leap_indicator =  ( flags[0] & ( LI0 | LI1 ) ) >> 6;
    hostMessage->version = (flags[0] & ( VN0 | VN1 | VN2 ) ) >> 3;
    hostMessage->mode = flags[0] & ( (MD0 | MD1 | MD2) );
    hostMessage->stratum = flags[1];
    hostMessage->poll = flags[2];
    hostMessage->precision = flags[3];
    hostMessage->root_delay = ntohl( networkMessage->root_delay );
    hostMessage->root_dispersion = ntohl( networkMessage->root_dispersion );
    hostMessage->ref_id = ntohl( networkMessage->ref_id );
    hostMessage->ref_timestamp_sec = ntohl( networkMessage->ref_timestamp_sec );
    hostMessage->ref_timestamp_frac = ntohl( networkMessage->ref_timestamp_frac );
    hostMessage->origin_timestamp_sec = ntohl( networkMessage->origin_timestamp_sec );
    hostMessage->origin_timestamp_frac = ntohl( networkMessage->origin_timestamp_frac );
    hostMessage->recv_timestamp_sec = ntohl( networkMessage->recv_timestamp_sec );
    hostMessage->recv_timestamp_frac = ntohl( networkMessage->recv_timestamp_frac );
    hostMessage->transmit_timestamp_sec = ntohl( networkMessage->transmit_timestamp_sec );
    hostMessage->transmit_timestamp_frac = ntohl( networkMessage->transmit_timestamp_frac );
}
void marshalMessage( NetworkMessage *networkMessage, HostMessage *hostMessage ) {
    int flags = 0;
    flags |= ( ( 1 << 29 ) | ( 1 << 25 ) | ( 1 << 24 ) );
    networkMessage->flags = htonl( flags );
    networkMessage->root_delay = htonl( hostMessage->root_delay );
    networkMessage->root_dispersion = htonl( hostMessage->root_dispersion );
    networkMessage->ref_id = htonl( hostMessage->ref_id );
    const time_t current = time( NULL );
    networkMessage->ref_timestamp_sec = htonl( current );
    networkMessage->ref_timestamp_frac = htonl( hostMessage->ref_timestamp_frac );
    networkMessage->origin_timestamp_sec = htonl( hostMessage->origin_timestamp_sec );
    networkMessage->origin_timestamp_frac = htonl( hostMessage->origin_timestamp_frac );
    networkMessage->recv_timestamp_sec = htonl( hostMessage->recv_timestamp_sec );
    networkMessage->recv_timestamp_frac = htonl( hostMessage->recv_timestamp_frac );
    networkMessage->transmit_timestamp_sec = htonl( hostMessage->transmit_timestamp_sec );
    networkMessage->transmit_timestamp_frac = htonl( hostMessage->transmit_timestamp_frac );
}

void calc_t1( Times *t ) {
    /** query the time using function given by requirements Ex. 5 (c) **/
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    t->t1_nsec = 1000000000 * tv.tv_sec + tv.tv_nsec;
}
void calc_t4( Times *t ) {
    /** query the time using function given by requirements Ex. 5 (c) **/
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    t->t4_nsec = 1000000000 * tv.tv_sec + tv.tv_nsec;
}
void calc_t2( Times *t , HostMessage *hostMessage) {
    /** Eine weitere Hürde bei der Implementierung auf Unix-artigen Betriebssystemen wie
     *  Linux ist der verwendete Zeitstempel. Dieser ist bei NTP die Anzahl von Sekunden seit
     *  dem 1.1.1900 und bei Unix die Anzahl der Sekunden seit dem 1.1.1970. Der Offest
     *  zwischen den beiden Zeitstempeln ist 2208988800 Sekunden, die zur Umrechnung
     *  jeweils addiert bzw. subtrahiert werden müssen.
     *  Außerdem ist das Format des NTPZeitstempels anders.
     *  Hier stellen die 64bit eine Fixpunktzahl dar und sind aufgeteils in
     *  32bits für die Sekunden und 32bit für die Fraction, also den Nachkommateil.
     *  **/
    t->t2_nsec = ( hostMessage->recv_timestamp_sec-2208988800 )*1000000000 + (int)(hostMessage->recv_timestamp_frac/ pow(2, 32)*1000000000);
}
void calc_t3( Times *t , HostMessage *hostMessage) {
    t->t3_nsec = ( hostMessage->transmit_timestamp_sec-2208988800 )*1000000000 + (int) (hostMessage->transmit_timestamp_frac/ pow(2, 32)*1000000000);
}
void calc_delay( Times *t ) {
    /** delay defined as RTT divided by 2 as in requirements Ex. 5 (e) **/
    t->delay_sec = (double)((t->t4_nsec - t->t1_nsec) - (t->t3_nsec - t->t2_nsec)) / 2000000000;
}
void calc_offset( Times *t ) {
    long tmp = ((t->t2_nsec - t->t1_nsec) + (t->t3_nsec - t->t4_nsec));
    t->offset_sec = (double) tmp/ (double) 2000000000;
}



