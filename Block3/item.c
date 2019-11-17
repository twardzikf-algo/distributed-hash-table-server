#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "item.h"

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

int item_free(Item *item) {
    if(item == NULL) return 1;
    else {
        free(item->key);
        free(item->value);
        free(item);
        return 0;
    }
}

void item_print(Item *item) {
    printf("Item( key: %s, value: %s)\n", item->key, item->value);
}