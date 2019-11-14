#ifndef ITEM_H_
#define ITEM_H_


/* In this file the item struct is implemented.
 * An item represents a single element in the hashtable, containing a key and its corresponding value
 * as well as the pointer to the next element, since each field in a hashtable is a linked list*/

typedef struct Item Item;

struct Item {
    char *key;
    char *value;
    Item* next;
};


/* item allocation  */
Item * item_create(char *key, char *value);
/* item freeing  */
int item_free(Item *item);
/* item printing */
void item_print(Item *item);

#endif