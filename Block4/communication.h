#pragma once

#define CONTROL 0
#define ACK 4
#define GET 5
#define SET 6
#define DEL 7
#define REPLY 6
#define LOOKUP 7
#define DHT_SIZE 256
#define GET_FLAG(X, BIT) (((X & (1 << BIT)) >> BIT)) // fkt uses boolsche Algebra to return 1 if and only if the bit on the BIT place (see PDF with protocol and bits on the top numbered 0...7) is set to 1, so like a getter for single bits in the first byte
// BIT can be {ACK, GET, SET, DEL etc}, these constants are just named  numbers to be at least a bit more readable, so SET_FLAG(msg_flags, DEL)  will cause the 4th bit in the char variable sg_flags to be set to 1
#define SET_FLAG(X, BIT) (X |= (1 << BIT)) // same here, weird voodo symbols, but its bitewise shift and Veroderung - the effect is that
#define UNSET_FLAG(X, BIT) (X &= ~( 1 << BIT )) // opposite effect to SET_FLAG() - ests given bit in given variable to 0

typedef struct _rpc_msg {
    unsigned char flags;
	unsigned short key_length;
	unsigned int value_length;
	char *key;
	char *value;
} rpc_msg;

typedef struct _control_msg {
    unsigned char flags;
    unsigned short hash_id;
    unsigned short node_id;
    unsigned int node_ip;
    unsigned short node_port;
} control_msg;

typedef struct _peer_info {
    int my_id;
    in_addr_t my_ip;
    int my_port;
    int pred_id;
    in_addr_t pred_ip;
    int pred_port;
    int succ_id;
    in_addr_t succ_ip;
    int succ_port;
} peer_info;

static inline unsigned int hash(char* key) { //copypasted from last year just for hash calcualtion
    unsigned int hash = 5381;
    for (int i = 0; i < strlen(key); ++key, ++i) hash = ((hash << 5) + hash) + (*key);
    return hash%DHT_SIZE;
}