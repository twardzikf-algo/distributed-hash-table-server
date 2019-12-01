#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "hashtable.c" /* https://gist.github.com/phsym/4605704 */
#include "communication.h"


void log_msg(char* text, char* ip, void* _msg) {
    if(GET_FLAG(((char*) _msg)[0], CONTROL)) {
        control_msg* msg = (control_msg* ) _msg;
        printf("%s:%s LOOKUP:%d REPLY:%d ",text, ip, GET_FLAG(msg->flags, LOOKUP), GET_FLAG(msg->flags, REPLY));
        printf("hash:%d id:%d ip:%d port:%d\n", msg->hash_id, msg->node_id, msg->node_ip, msg->node_port);
    } else {
        rpc_msg* msg = (rpc_msg*) _msg;
        printf("%s:%s ACK:%d ", text, ip, GET_FLAG(msg->flags, ACK));
        printf("GET:%d SET:%d DEL:%d",GET_FLAG(msg->flags, GET), GET_FLAG(msg->flags, SET), GET_FLAG(msg->flags, DEL));
        printf(" key/len: %s/%d value/len:%s/%d\n", msg->key, msg->key_length, msg->value, msg->value_length);
    }
}
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
int is_my_succ_responsible(peer_info peer_info, int hash_id) {
    return (peer_info.my_id < peer_info.succ_id)
           ? (hash_id > peer_info.my_id && hash_id <= peer_info.succ_id)
           : (hash_id > peer_info.my_id && hash_id < DHT_SIZE) || (hash_id >= 0 && hash_id <= peer_info.succ_id);
}

void marshal_and_send(void* msg, int send_sock) {
    rpc_msg* rpc_msg = NULL;

    rpc_msg = (rpc_msg*) msg;

    control_msg* control_msg = (control_msg*) msg;
    if (GET_FLAG(rpc_msg->flags, CONTROL)) {
        control_msg->hash_id = htons(control_msg->hash_id);
        control_msg->node_id = htons(control_msg->node_id);
        control_msg->node_ip = htonl(control_msg->node_ip);
        control_msg->node_port = htons(control_msg->node_port);
        send(send_sock, control_msg, 11, 0);
    } else {
        unsigned short key_length = rpc_msg->key_length;
        unsigned int value_length = rpc_msg->value_length;
        rpc_msg->key_length = htons(rpc_msg->key_length);
        rpc_msg->value_length = htonl(rpc_msg->value_length);
        send(send_sock, rpc_msg, 7, 0);
        if (msg->key != NULL) send(send_sock, rpc_msg->key, key_length, 0);
        if (msg->value != NULL) send(send_sock, rpc_msg->value, value_length, 0);
    }
}

void* recv_and_unmarshal(int recv_sock) {
    unsigned char flags = 0;
    recv(recv_sock, &flags, 1, 0);
    if (GET_FLAG(flags, CONTROL)) {
        control_msg* msg =  calloc(1, sizeof(control_msg));
        recv(recv_sock, &flags, 10, 0);
        msg->flags = flags;
        msg->hash_id = ntohs(msg->hash_id);
        msg->node_id = ntohs(msg->node_id);
        msg->node_ip = ntohl(msg->node_ip);
        msg->node_port = ntohs(msg->node_port);
        return msg;
    } else {
        rpc_msg* msg =  calloc(1, sizeof(rpc_msg));
        recv(recv_sock, msg, 6, 0);
        msg->flags = flags;
        msg->key_length = htons(msg->key_length);
        msg->value_length = htonl(msg->value_length);
        msg->key = calloc(1, msg->key_length + 1);
        msg->value = calloc(1, msg->value_length + 1);
        recv(recv_sock, msg->key, msg->key_length, 0);
        recv(recv_sock, msg->value, msg->value_length, 0);
        return msg;
    }
}

void handle_request(hashtable_t *db, rpc_msg* msg) {
    msg->value_length = 0;
    if (GET_FLAG(msg->flags, GET)) {
        if ((msg->value = ht_get(db, msg->key)) == NULL) UNSET_FLAG(msg->flags, GET);
        msg->value_length = strlen(msg->value);
    }
    if (GET_FLAG(msg->flags, SET)) {
        ht_put(db, msg->key, msg->value);
    }
    if (GET_FLAG(msg->flags, DEL)) {
        if(ht_get(db, msg->key) == NULL) UNSET_FLAG(msg->flags, DEL);
        free(ht_remove(db, msg->key));
    }
    SET_FLAG(msg->flags, ACK);
}

int main(int argc, char *argv[]) {
    if (argc != 10) {
        printf("Wrong usage: ./peer <MY_ID> <MY_IP> <MY_PORT> <PRED_ID> <PRED_IP> <PRED_PORT> <SUCC_ID> <SUCC_IP> <SUCC_PORT>");
        return 1;
    }

    peer_info peer_info = { 0 };
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
        void* in_msg = recv_and_unmarshal(client_sock);
        void* out_msg;
        log_msg("recv_from", inet_ntoa(client_addr.sin_addr), in_msg);

        if (GET_FLAG(in_msg->flags, CONTROL)) {
            in_msg = (control_msg*) in_msg;

            if (GET_FLAG(in_msg->flags, LOOKUP)) {
                out_msg = (control_msg*) in_msg;
                if(is_my_succ_responsible(peer_info, in_msg->hash_id)) {
                    UNSET_FLAG(out_msg->flags, LOOKUP);
                    SET_FLAG(out_msg->flags, REPLY);
                    ((control_msg*) out_msg)->node_id = peer_info.succ_id;
                    ((control_msg*) out_msg)->node_ip = peer_info.succ_ip;
                    ((control_msg*) out_msg)->node_port = peer_info.succ_port;
                    // choose a socket to the node whose ip/port is in in_msg
                } else {
                    // choose a socket/connection to my successor
                }
            }
            if (GET_FLAG(in_msg->flags, REPLY)) {
                out_msg = (rpc_msg*) calloc(1, sizeof(rpc_msg));
                // how to deal with that shieeeet
            }
        } else {
            in_msg = (rpc_msg*) in_msg;
            if (GET_FLAG(((rpc_msg*) in_msg)->flags, ACK)) {
                // TODO choose awaiting client socket
            } else {
                if(i_am_responsible(peer_info, hash(((rpc_msg*) in_msg)->key))) {
                    out_msg = in_msg;
                    handle_request(db, out_msg);
                }
                else {
                    out_msg = calloc(1, sizeof(control_msg));
                    SET_FLAG(((control_msg*) out_msg)->flags, CONTROL);
                    SET_FLAG(((control_msg*) out_msg)->flags, LOOKUP);
                    ((control_msg*) out_msg)->hash_id = hash(((rpc_msg*) in_msg)->key);
                    ((control_msg*) out_msg)->node_id = peer_info.my_id;
                    ((control_msg*) out_msg)->node_ip = peer_info.my_ip;
                    ((control_msg*) out_msg)->node_port = peer_info.my_port;
                }
                free(((rpc_msg*) in_msg)->key);
            }
        }
        log_msg("send_to", inet_ntoa(client_addr.sin_addr), out_msg);
        marshal_and_send(out_msg, client_sock);
        free(in_msg);
        if(out_msg != NULL) free(out_msg);
        close(client_sock);
    }

    close(server_sock);
    ht_destroy(db);
    return 0;
}