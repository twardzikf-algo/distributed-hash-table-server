#ifndef ITEM_H_
#define ITEM_H_

typedef struct Item Item;

struct Item {
    char *key;
    char *value;
    Item* next;
};

Item * item_create(char *key, char *value);
int item_free(Item *item);
void item_print(Item *item);

#endif