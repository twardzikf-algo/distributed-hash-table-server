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
#include "hashtable.c" /* https://gist.github.com/phsym/4605704 */

int setup_server_connection(char* argv[]) {
    struct sockaddr_in server_address = { .sin_family = AF_INET, .sin_port = htons(atoi(argv[3])), .sin_addr.s_addr = htonl(INADDR_ANY) };
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) return -1;
    if ((bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address))) < 0) return -2;
    if (listen(server_sock, 16) < 0) return -4;
    return server_sock;
}
int i_am_responsible(peer_info peer_info, int hash_id) {
    return (peer_info.pred_id < peer_info.my_id)
           ? (hash_id > peer_info.pred_id && hash_id <= peer_info.my_id)
           : (hash_id > peer_info.pred_id && hash_id < DHT_SIZE) || (hash_id >= 0 && hash_id <= peer_info.my_id);
}
int get(hashtable_t *db, rpc_msg* msg) {
    if ((msg->value = ht_get(db, msg->key)) == NULL) return 1;
    msg->value_length = strlen(msg->value);
    return 0;
}
int set(hashtable_t *db, rpc_msg* msg, int client_sock) {
    msg->value = (char*) malloc(msg->value_length + 1);
    read(client_sock, msg->value, msg->value_length);
    msg->value[msg->value_length] = '\0';
    ht_put(db, msg->key, msg->value);
    return 0;
}
int del(hashtable_t *db, rpc_msg* msg) {
    if(ht_get(db, msg->key) == NULL) return 1;
    free(ht_remove(db, msg->key));
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 10) {
        printf("Wrong usage: ./peer <MY_ID> <MY_IP> <MY_PORT> <PRED_ID> <PRED_IP> <PRED_PORT> <SUCC_ID> <SUCC_IP> <SUCC_PORT>");
        return 1;
    }

    peer_info peer_info = { 0 }; // declaring and zeroing struct for infos about ip, id, port of me and my sucessor and predecessor (so I can communicate with them)
    // however for now in BLock4 only successor is used
    // down: pack each information straight from the array of cmd to struct fields while formatting and converting from strings to ints on the fly
    peer_info.my_id = atoi(argv[1]);
    peer_info.my_ip = inet_addr(argv[2]);
    peer_info.my_port = atoi(argv[3]);
    peer_info.pred_id = atoi(argv[4]);
    peer_info.pred_ip = inet_addr(argv[5]);
    peer_info.pred_port = atoi(argv[6]);
    peer_info.succ_id = atoi(argv[7]);
    peer_info.succ_ip = inet_addr(argv[8]);
    peer_info.succ_port = atoi(argv[9]);

    // create a hashtable that will hold our key-value pairs
    hashtable_t *db = ht_create(DHT_SIZE);

    // create a socket on which we will listen for connections
    int server_sock = setup_server_connection(argv);
    if (server_sock == -1) return 1;
    struct sockaddr_in client_addr;
    unsigned int client_addr_len = 0;

    while (1) {
        int client_sock;
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) return 1; //accept a connection from a client, if it fails, terminate

        unsigned char msg_flags = 0; // initialzie a char for flags
        // we only read one byte from teh client first
        // because basing on the first byte (flags) the rest of the msg will vary in size
        read(client_sock, &msg_flags, 1);

        // now we have the falgs so we can actually interpret what kind of message it is and how to react to it
        if (GET_FLAG(msg_flags, CONTROL)) { // it is a control message
            control_msg incoming_msg = {0}; // declaring and zeroing the struct for the rest of the msg
            read(client_sock, &incoming_msg, 10); // read rest of the bytes (it is a control msg, so always fixed in size (see PDF) so we can read straight 10 bytes and done)
            if (GET_FLAG(msg_flags, LOOKUP)) { // it is a control lookup message
                if(i_am_responsible(peer_info, incoming_msg.hash_id)) { // i am responsible for that hash id
                    // TODO handle the request and answer to the node (its ip and port is written in incoming_msg)
                } else { // I am not responsible for that hash id
                    // TODO forward the lookup to my successor
                }
            }
            if (GET_FLAG(msg_flags, REPLY)) { // now I know who is responsible for the hash id I need
                // TODO send the request for actual data to the node that is responsible for it
            }
        } else { // IT IS A "RemoteProcedureCall" message - that means the first 'usual' type: it can be either a request from client or a response from one of my peers
            rpc_msg incoming_msg = {0}; // declaring and zeroing the struct for the rest of the msg
            read(client_sock, &incoming_msg, 6); // here we only know for sure that at least 6 bytes are coming, so we read them first
            // we have these 6 bytes, so we also have the key_length (key is always there  independently from the request type (value however is only needed by SET so we only read it there))
            incoming_msg.key = (char*) malloc(incoming_msg.key_length + 1);
            read(client_sock, incoming_msg.key, incoming_msg.key_length);
            incoming_msg.key[incoming_msg.key_length] = '\0'; // allocate some Lebensraum for the key
            log_rpc_msg(inet_ntoa(client_addr.sin_addr), incoming_msg, msg_flags); //  throw shit out on the screen for some transparency

            // TODO actually probably i should check here first if the ACK bit is set, because if so, that means the message is already coming back from a peer and handled so I only have to froward it to the client
            if(i_am_responsible(peer_info, hash(incoming_msg.key))) { // I am responsible for  the key, bingo
                incoming_msg.key_length = 0;
                incoming_msg.value_length = 0;
                if (GET_FLAG(msg_flags, GET) == 1 && get(db, &incoming_msg)) UNSET_FLAG(msg_flags, GET);
                if (GET_FLAG(msg_flags, SET) == 1) set(db, &incoming_msg, client_sock);
                if (GET_FLAG(msg_flags, DEL) == 1 && del(db, &incoming_msg)) UNSET_FLAG(msg_flags, DEL);
                SET_FLAG(msg_flags, ACK);
                send(client_sock, &msg_flags, 1, 0);
                send(client_sock, &incoming_msg, 6, 0);
                if (GET_FLAG(msg_flags, GET) == 1 && incoming_msg.value != NULL)
                    send(client_sock, incoming_msg.value, incoming_msg.value_length, 0);
            } else {
                SET_FLAG(msg_flags, CONTROL);
                SET_FLAG(msg_flags, LOOKUP);
                /*control_msg lookup_msg = {
                        .hash_id = hash(incoming_msg.key),
                        .node_id = peer_info.my_id,
                        .node_ip = peer_info.my_ip,
                        .node_port = peer_info.my_port
                };*/
                struct sockaddr_in server_address = { .sin_family = AF_INET, .sin_port = htons(atoi(argv[2])), .sin_addr.s_addr = inet_addr(argv[1]) };
                int client_socket = socket(PF_INET, SOCK_STREAM, 0);
                connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));
                printf("sending a lookup msg to my successor\n");
                // TODO actually send the lookup msg to my successor
             }
            free(incoming_msg.key);
        }
        close(client_sock);
    }

    close(server_sock);
    ht_destroy(db);
    return 0;
}