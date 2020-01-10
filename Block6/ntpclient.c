#include "lib/ntplib.c"

#define SERVERS 5

int main(int argc, char *argv[]) {
    
    char *servers[SERVERS];
    servers[0] = "ntp0.rrze.uni-erlangen.de";
    servers[1] = "time1.uni-paderborn.de";
    servers[2] = "stratum2-4.NTP.TechFak.Uni-Bielefeld.DE";
    servers[3] = "0.de.pool.ntp.org";
    servers[4] = "1.de.pool.ntp.org";
    
    int iterations = SERVERS;
    long best_server_delay = 9223372036854775807;
    int best_server_index = 0;
    int best_server_offset = 0;
    
   

    for( int i=0 ; i< iterations ; i++ ) {
        
        Connection* connection = connection_setup( UDP, servers[i% SERVERS], "123", 0 );
        
        char *buffer = NULL;
        HostPacket *hostPacket = calloc( 1, sizeof( HostPacket ) );
        NetworkPacket *networkPacket = calloc( 1, sizeof( NetworkPacket ) );
        RequestTimes *times = calloc( 1, sizeof( RequestTimes ) );
        
        marshal_packet( networkPacket, hostPacket );
        buffer = (char*) networkPacket;
        
        set_t1( times );
        
        connection_send_udp( connection, buffer );
        connection_recv_udp( connection, buffer );
        unmarshal_packet( networkPacket, hostPacket );
        
        set_t4( times );
        set_t2( times , hostPacket );
        set_t3( times , hostPacket );
        set_delay( times );
        set_offset( times );
        if( times->delay_microsec < best_server_delay ) {
            best_server_delay = times->delay_microsec;
            best_server_index = i%SERVERS;
            best_server_offset = times->offset_microsec;
        }
        printf("\t  [current server]: %s\n", servers[i% SERVERS] );
        printf("\t  [offset]:         %ld\n", times->offset_microsec );
        printf("\t  [delay]:          %ld\n", times->delay_microsec );
        
        /* for debugging */
        //print_packet( hostPacket );
        //printf("t2: %ld, t3: %ld\n", times->t2_microsec, times->t3_microsec );
        //printf("t3-t2 : %ld\n", times->t3_microsec-times->t2_microsec );
        
        free( hostPacket );
        free( networkPacket );
        free( times );
        connection_close( connection );
        sleep( 1);
    }
    
    printf("[client]: Results:\n");
    printf("\t  [best server]:    %s\n", servers[best_server_index] );
    printf("\t  [best delay]:     %ld\n", best_server_delay );
    printf("\t  -------------------------------------------------\n");
    
    struct timeval tv;
    gettimeofday(&tv,NULL);
    const time_t current_sec = tv.tv_sec;
    long current_milisec = tv.tv_usec / 1000;
    long current_microsec =  1000000 * tv.tv_sec + tv.tv_usec;
    
    long adjusted_microsec = current_microsec + best_server_offset;
    const time_t adjusted_sec = adjusted_microsec / 1000000;
    long adjusted_milisec = (adjusted_microsec / 1000) - adjusted_sec*1000;
    
    printf("\t  [current local time]:  %s\t\t\t\t\t\t\t  miliseconds:%ld\n", ctime( &current_sec ), current_milisec );
    printf("\t  [adjusted local time]: %s\t\t\t\t\t\t\t  miliseconds:%ld\n", ctime( &adjusted_sec ), adjusted_milisec );
    printf("\t  -------------------------------------------------\n");
    
    return 0;
}
