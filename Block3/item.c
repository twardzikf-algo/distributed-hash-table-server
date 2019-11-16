#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "item.h"


/* create an item = allocate memory for the struct
 * and assign char pointers given as parameters to the elements of the struct */
Item* item_create(char *key, char *value) {
    Item *item = malloc(sizeof(Item));
    if( item != NULL) {
        item->key = key;
        item->value = value;
        if(item->key == NULL || item->value == NULL) {
            free(item);
            return NULL;
        }
    }
    return item;
}

/* free the item  = free its key and value pointers and free the item struct itself */
int item_free(Item *item) {
    if(item == NULL) return 1;
    else {
        free(item->key);
        free(item->value);
        free(item);
        return 0;
    }
}

/* print the state of an item in a compact, readable way */
void item_print(Item *item) {
    printf("Item( key: %s, value: %s)\n", item->key, item->value);
}

