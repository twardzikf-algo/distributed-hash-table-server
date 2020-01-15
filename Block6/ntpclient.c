#include "lib/ntplib.c"

int main(int argc, char *argv[]) {

    /** correct format for the arguments given by requirements Ex. 5 (a) **/
    if (argc < 3) {
        printf("Wrong usage!\nCorrect usage: ./ntpclient n server1 server2 server3...\n");
        return 0;
    }

    int iterations = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        char* server = argv[i];
        double max_delay = 0.0;
        double min_delay= 999999.0;
        for( int j=0 ; j < iterations ; j++ ) {
            Connection* connection = setupConnection(server, "123");
            char *buffer = NULL;
            HostMessage *hostMessage = calloc( 1, sizeof( HostMessage ) );
            NetworkMessage *networkMessage = calloc( 1, sizeof( NetworkMessage ) );
            marshalMessage( networkMessage, hostMessage );
            buffer = (char*) networkMessage;
            Times *times = calloc( 1, sizeof( Times ) );

            /** set T1 just before sending the message to NTP server */
            calc_t1(times);

            /** 48 bytes is the size of the ntp message we are sending **/
            sendto(connection->sockfd, buffer, 48, 0, connection->addrinfo->ai_addr, connection->addrinfo->ai_addrlen);
            recvfrom(connection->sockfd, buffer, 48, 0, connection->addrinfo->ai_addr, &connection->addrinfo->ai_addrlen);

            unmarshalMessage(networkMessage, hostMessage);
            /** set T4 immedietely upon receiving and unmarshalling the message**/
            calc_t4(times);

            /** calculate T2 and T3 from the message we just received**/
            calc_t2(times, hostMessage);
            calc_t3(times, hostMessage);
            calc_delay(times);
            calc_offset(times);
            if (times->delay_sec > max_delay) max_delay = times->delay_sec;
            if (times->delay_sec < min_delay) min_delay = times->delay_sec;
            double root_dispersion_sec = ((double)hostMessage->root_dispersion) / 1000000;
            double aggr_dispersion_sec = max_delay - min_delay;
            /** Print infos for each query in the format given by requirements Ex. 5 (g)
             * '%0.6lf' is notation for a placeholder of type double (hence lf)
             *          and 6 digits after comma (hence 0.6)
             *          see: https://www.geeksforgeeks.org/g-fact-41-setting-decimal-precision-in-c/
             *
             * 'j'      number of query, starting at 0 given by requirements Ex. 5 (d)
             *
             * 'root_dispersion_sec' is root dispersion converted from microsec to sec
             *
             * 'aggr_dispersion_sec' is dispersion of our client to the server over all queries
             *          (changes as more queries are executed)
             *          for final decision on definition of dispersion see:
             *  https://isis.tu-berlin.de/mod/forum/discuss.php?d=257914
             *  https://isis.tu-berlin.de/mod/forum/discuss.php?d=257854
             * **/
            printf("%s;%d;%0.6lf;%0.6lf;%0.6lf;%0.6lf\n",
                    server,
                    j,
                    root_dispersion_sec,
                    aggr_dispersion_sec,
                    times->delay_sec,
                    times->offset_sec
                    );

            /** free all allocated structs **/
            free(hostMessage);
            free(networkMessage);
            free(times);
            closeConnection(connection);

            /** wait 8 sec after each query given by requirements Ex. 5 (b) **/
            sleep(1);
        }
    }
    return 0;
}
