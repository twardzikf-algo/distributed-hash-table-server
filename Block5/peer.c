/*
 * Block4 - peer.c
 * Abgabe der Gruppe T0607
 * 2019-12-01 16:51
 */

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>     // for INT_MAX
#include <sys/time.h>
#include <sys/types.h>

#include "err.h"
#include "net.h"
#include "protocol.h"
#include "table.h"

#define USAGE_STRING " <my_id> <my_ip> <my_port> <prev_id> <prev_ip> <prev_port> <next_id> <next_ip> <next_port>"

int listen_socket = -1;
fd_set master;
int i = -1; // loop variable that is used when iterating over sockets from select

void* request = NULL;
void* response = NULL;

pair* table = NULL;
socket_pair* socket_table_by_key = NULL;
socket_pair* socket_table = NULL;

void cleanup();
void INTHandler();
void handle_control_msg(void* req_mem, packet req, int range, const char* argv[]);
void* execute_hash_req(void* req_mem, packet* req, size_t* resp_len);

void cleanup() {
    free(request);
    request = NULL;
    free(response);
    response = NULL;
    if (i != -1) {
        close(i);
        FD_CLR(i, &master);
    }
}

// handle termination when waiting for connection.
void INTHandler() {
    if (listen_socket != -1) close(listen_socket);
    cleanup();
    free_table(table);
    close_table_by_key(socket_table_by_key);
    close_table(socket_table);
    exit(EXIT_SUCCESS);
}

void handle_control_msg(void* req_mem, packet req, int range, const char* argv[]) {
    packet resp = init_packet_empty();
    switch(req.type) {
        case LKP:
            switch (range) {
                case 2: // reply with next_id
                    DEBUG("Next peer is holding key. Responding to sender of lookup.\n");
                    resp.type = LKP_RPLY;
                    resp.hash_id = req.hash_id;
                    resp.node_id = atoi(argv[NEXT_ID]);
                    resp.node_ip = string_to_ip4(argv[NEXT_IP]);
                    resp.node_port = atoi(argv[NEXT_PORT]);

                    char* sender_ip = ip4_to_string(req.node_ip);
                    char* sender_port = port_to_string(req.node_port);

                    int socketfd = build_connection_as_client(sender_ip, sender_port);
                    free(sender_ip);
                    free(sender_port);
                    if (-1 == socketfd) {
                        // error message printed already
                        return;
                    }

                    void* lkp_response;
                    size_t len;
                    if (!(lkp_response = build_packet(resp, &len))) {
                        // error message printed already
                        close(socketfd);
                        return;
                    }

                    sendn(socketfd, lkp_response, len, 0);
                    // error handling would do the same as we do in next few lines anyway
                    //      and thus error handling is not necessary

                    close(socketfd);
                    free(lkp_response);
                    break;
                case 0: // forward message
                    DEBUG("Peer that holds this key is not known. Asking next peer.\n");
                    ask_next(req_mem, argv[NEXT_IP], argv[NEXT_PORT]);
                    // no error handling necessary
                    break;
                default:
                    ERROR("Something went wrong.\n");
                    break;
            }
            break;
        default:
            ERROR("Unknown control message.\n");
            break;
    }
}

// Execute request on hash table.
void* execute_hash_req(void* req_mem, packet* req, size_t* resp_len) {
    packet resp = init_packet_empty();
    pair* req_pair = NULL;
    switch(req->type) {
        case GET_REQ:
            // get value by key in hash table
            HASH_FIND(hh, table, req->key, req->key_len, req_pair);
            if (!req_pair) {
                // No pair with key exists
                ERROR("Key does not exist. Responding with empty body.\n");
                resp.type = GET_ACK;
                free(req->key);
            } else {
                // Take key from request and value from hash table find
                resp = init_hash_packet(GET_ACK, req->key_len, req->key, req_pair->value_len, req_pair->value);
            }
            break;
        case SET_REQ:
            // look first if already exists
            HASH_FIND(hh, table, req->key, req->key_len, req_pair);
            if (req_pair) {
                // Delete if existing
                HASH_DEL(table, req_pair);
                free_pair(req_pair);
            }

            req_pair = init_pair(*req);
            if (!req_pair) {
                ERROR("calloc error (in init_pair): %s\n", strerror(errno));
                free(req->key);
                free(req->value);
                return NULL;
            }
            HASH_ADD_KEYPTR(hh, table, req_pair->key, req_pair->key_len, req_pair);

            resp.type = SET_ACK;
            break;
        case DEL_REQ:
            // Check if <key, value> pair exists already
            HASH_FIND(hh, table, req->key, req->key_len, req_pair);
            if (req_pair) {
                // Delete if existing
                HASH_DEL(table, req_pair);
                free_pair(req_pair);
            } else {
                // Client will not see the difference
                DEBUG("Key does not exist. Still sending acknowlegment.\n");
            }
            resp.type = DEL_ACK;
            free(req->key);
            break;
        default:
            ERROR("Unknown request.\n");
            return NULL;
    }
    void* resp_pkg = build_packet(resp, resp_len);
    free(resp.key);
    return resp_pkg;
}

int main(int argc, const char* argv[]) {
    if (argc != 10) {
        ERROR_EXIT("Usage: %s %s\n", argv[0], USAGE_STRING);
    }

    FD_ZERO(&master);
    int fdmax;

    listen_socket = build_connection_as_server(argv[MY_PORT]);
    if (listen_socket == -1) {
        // error message already printed
        // cleanup not necessary
        exit(EXIT_FAILURE);
    }
    FD_SET(listen_socket, &master);
    fdmax = listen_socket;

    signal(SIGINT, INTHandler);
    while (1) {
        fd_set read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            ERROR("select error: %s\n", strerror(errno));
            INTHandler();
        }
        // Since new sockets are added into master, they will not be set in read_set (in this while iteration)
        // and thus we also do not need to iterate over them
        int fdmax_copy = fdmax;
        // loop over the existing connections looking for data to read
        for (i = 0; i <= fdmax_copy; i++) {
            if (!FD_ISSET(i, &read_fds)) {
                continue;
            }
            if (i == listen_socket) {
                // new connection
                int accepted_socket = accept_connection(listen_socket);
                if (accepted_socket == -1) {
                    // error message already printed
                    // cleanup not necessary
                    continue;
                }
                DEBUG("Connection accepted.\n");
                // add new socket to master set
                FD_SET(accepted_socket, &master);
                if (accepted_socket > fdmax) {
                    // update max fd
                    fdmax = accepted_socket;
                }
                continue;
            }
            // hande data from a client
            packet req;
            size_t req_size;
            if (!(request = recv_packet(i, &req, &req_size))) {
                if (errno == ENOMEM) {
                    // we are out of memory
                    ERROR("malloc error: %s\n", strerror(errno));
                    INTHandler();
                } else {
                    // error message printed already
                    // delete socket from internal hash table
                    socket_pair* s;
                    HASH_FIND(hh_sock, socket_table, &i, sizeof(int), s);
                    if (s) {
                        HASH_DELETE(hh_sock, socket_table, s);
                        close_pair(s);
                    }

                    cleanup();
                    continue;
                }
            }

            // get hash_key from memory (depending on the type it lays at different locations)
            HASH_ID_TYPE hash_key = get_ring_key(req.key_len, request + (is_known_hash_type(req.type) ? HEADER_LEN : HASH_ID_OFFSET));
            // Only copy into the packet struct, if we are responsible for this key.
            // If not it is suffiecient to only set pointers since we will just forward the request
            int range = what_range(hash_key, atoi(argv[MY_ID]), atoi(argv[PREV_ID]), atoi(argv[NEXT_ID]));
            int do_memcpy = 0;
            if (is_known_req_type(req.type) && range == 1) {
                do_memcpy = 1;
            }

            // initialize body struct from request body memory
            if (!extract_body(request, &req, do_memcpy)) {
                // error message printed already
                // delete socket from internal hash table
                socket_pair* s;
                HASH_FIND(hh_sock, socket_table, &i, sizeof(int), s);
                if (s) {
                    HASH_DELETE(hh_sock, socket_table, s);
                    close_pair(s);
                }

                cleanup();
                continue;
            }

            if (is_known_control_req(req.type)) {
                // LOOKUP

                handle_control_msg(request, req, range, argv);
                cleanup();
                continue;
            } else if (is_known_control_rsp(req.type)) {
                // LOOKUP - Reply

                // search in table and find correct request
                socket_pair* accepted_socket_pair;
                HASH_FIND(hh_key, socket_table_by_key, &hash_key, sizeof hash_key, accepted_socket_pair);
                if (!accepted_socket_pair) {
                    ERROR("Something went wrong. We did not send a lookup with this hash-id.\n");
                    cleanup();
                    continue;
                }
                if (!accepted_socket_pair->packet || !accepted_socket_pair->packet_len) {
                    ERROR("Something went wrong. The socket_pair does not contain a packet that we can forward.\n");
                    close_pair(accepted_socket_pair);
                    cleanup();
                    continue;
                }
                // forward request to peer in message
                char* key_holder = ip4_to_string(req.node_ip);
                char* key_holder_port = port_to_string(req.node_port);
                int socketfd = forward_packet(key_holder, key_holder_port, accepted_socket_pair->packet, accepted_socket_pair->packet_len);
                free(key_holder);
                free(key_holder_port);
                if (socketfd == -1) {
                    // error message printed already
                    cleanup();
                    continue;
                }
                // Since we are waiting for a response we need to add the socket to master
                FD_SET(socketfd, &master);
                if (socketfd > fdmax) {
                    // update max fd
                    fdmax = socketfd;
                }
                // And since we also need the connection between the hash_key/client-request we need to also add it to our table
                socket_pair* key_holder_pair = init_socket_pair(socketfd, hash_key, 4, NULL, 0);
                HASH_ADD(hh_sock, socket_table, socketfd, sizeof(int), key_holder_pair);

                free(accepted_socket_pair->packet);
                accepted_socket_pair->packet = NULL;
                accepted_socket_pair->packet_len = 0;
                cleanup();
                continue;
            } else if (is_known_req_type(req.type)) {
                // GET, SET, DELETE

                char* key_holder;
                char* key_holder_port;
                switch(range) {
                    case 1: { // We hold this key
                        DEBUG("Key lays in our table. Processing request directly.\n");
                        size_t resp_len;
                        if (!(response = execute_hash_req(request, &req, &resp_len))) {
                            // error message already printed
                            cleanup();
                            continue;
                        }
                        // respond
                        if (sendn(i, response, resp_len, 0)) {
                            DEBUG("Repsonse sent.\n");
                        }
                        // We do not care if send is succesful or not
                        cleanup();
                        continue;
                    } case 2: { // Next peer holds key
                        DEBUG("Next peer holds key. Forwarding request.\n");
                        // forward request to next node
                        key_holder = (char*)argv[NEXT_IP];
                        key_holder_port = (char*)argv[NEXT_PORT];

                        DEBUG("Building new connection as client.\n");
                        // forward request
                        int socketfd;
                        if (-1 == (socketfd = forward_packet(key_holder, key_holder_port, request, req_size))) {
                            // error message printed already
                            cleanup();
                            continue;
                        }
                        // Since we are waiting for a respond we need to add both sockets to our internal hash table
                        socket_pair* accepted_socket_pair = init_socket_pair(i, hash_key, 2, NULL, 0);
                        HASH_ADD(hh_sock, socket_table, socketfd, sizeof(int), accepted_socket_pair);
                        HASH_ADD(hh_key, socket_table_by_key, hash_key, sizeof hash_key, accepted_socket_pair);
                        FD_SET(socketfd, &master);
                        if (socketfd > fdmax) {
                            // update max fd
                            fdmax = socketfd;
                        }
                        socket_pair* key_holder_pair = init_socket_pair(socketfd, hash_key, 4, NULL, 0);
                        HASH_ADD(hh_sock, socket_table, socketfd, sizeof(int), key_holder_pair);
                        // Protect socket from being closed
                        int i_bkp = i;
                        i = -1;
                        cleanup();
                        i = i_bkp;
                        continue;
                    } default: { // we do not know who holds this key
                        DEBUG("Unknown key. Sending lookup to next peer.\n");
                        // send lookup to next node
                        void* lookup_pkg;
                        if (!(lookup_pkg = create_lookup(get_ring_key(req.key_len, req.key), argv[MY_ID], argv[MY_IP], argv[MY_PORT]))) {
                            // error message printed already
                            cleanup();
                            continue;
                        }
                        if (!ask_next(lookup_pkg, argv[NEXT_IP], argv[NEXT_PORT])) {
                            // error message printed already
                            free(lookup_pkg);
                            cleanup();
                            continue;
                        }
                        DEBUG("Lookup sent.\n");
                        free(lookup_pkg);

                        // Since we are waiting for a respond we need to add the socket to our internal hash table
                        socket_pair* accepted_socket_pair = init_socket_pair(i, hash_key, 2, request, req_size);
                        HASH_ADD(hh_sock, socket_table, socketfd, sizeof(int), accepted_socket_pair);
                        HASH_ADD(hh_key, socket_table_by_key, hash_key, sizeof hash_key, accepted_socket_pair);
                        // Protect socket from being closed
                        request = NULL;
                        int i_bkp = i;
                        i = -1;
                        cleanup();
                        i = i_bkp;
                        break;
                    }
                }
            } else if (is_known_ack_type(req.type)) {
                // GET-, SET-, DELETE- acknowlegments

                // this has to be the respond to the forwarded request
                socket_pair* key_holder_pair;
                // search in hash table
                HASH_FIND(hh_sock, socket_table, &i, sizeof(int), key_holder_pair);
                if (!key_holder_pair) {
                    ERROR("Something went wrong. We did not send a request to this peer.\n");
                    cleanup();
                    continue;
                }
                // find client_socket
                socket_pair* client_pair;
                socket_pair* tmp = socket_table_by_key;
                do {
                    HASH_FIND(hh_key, tmp, &key_holder_pair->hash_key, sizeof key_holder_pair->hash_key, client_pair);
                    if (!client_pair) {
                        ERROR("Something went wrong. We can't find the socket to the client.\n");
                        cleanup();
                        continue;
                    }
                    // Since the hash_key is saved in two entrys I could happen that we not find the client socket first.
                    // In this case we need to continue searching behind forward_pair
                    // (I'm pretty sure that this case will never happen in practice because we add the client socket first to the table,
                    // but better be safe that sorry)
                    tmp = client_pair->hh_key.next;
                } while (client_pair == key_holder_pair);

                // forward reply
                DEBUG("Forwarding response to client.\n");
                sendn(client_pair->socketfd, request, req_size, 0);
                // we don't care about errors in sendn. We are doing the same thing in both cases

                // close, remove from master and from hash_table the client and the key_holder socket
                close(client_pair->socketfd);
                FD_CLR(client_pair->socketfd, &master);
                HASH_DELETE(hh_sock, socket_table, key_holder_pair);
                close_pair(key_holder_pair);
                HASH_DELETE(hh_key, socket_table_by_key, client_pair);
                HASH_DELETE(hh_sock, socket_table, client_pair);
                close_pair(client_pair);

                cleanup();
                continue;
            } else {
                ERROR("Unknown type.\n");
                cleanup();
                continue;
            }
        } // end for loop [0, fdmax]
    } // end of main accept loop (while)
} // end of main
