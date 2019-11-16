#pragma once 

char *get(char *key);
void set(char *key, char *value);




#define INTERN 1
#define ACK 4 
#define GET 5
#define SET 6
#define DEL 7

#define READ_FLAGS(X, BIT) (((X & (1 << BIT)) >> BIT))
#define SET_FLAGS(X, BIT) (X |= (1 << BIT))

/*
	char *(*get)(char *key);
	void (*set)(char *key, char *value);*/


typedef struct _comm_nessage {
	unsigned char function;
	unsigned char trans_id;
	unsigned char key_length[2];
	unsigned char value_length[2];
	unsigned char id[2];
	unsigned char ip[4];
	unsigned char port[2];
	char *key;
	char *value;	
}comm_nessage;

static inline int two_byte_int(unsigned char *bytes) 
{
    return bytes[0] | bytes[1] << 8;
}
