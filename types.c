#include <string.h>
#include <stdlib.h>

#include "types.h"

ptr_array_t* ptr_array_new(void) {
    ptr_array_t *pta = malloc(sizeof(ptr_array_t));
    pta->len = 0;
    pta->data = NULL;
    return pta;
}

void ptr_array_add(ptr_array_t *pta, void *ptr) {
    if (!pta->len)
        pta->data = malloc(sizeof(void *));
    else
        pta->data = realloc(pta->data, sizeof(void *) * (pta->len + 1));
    
    pta->data[pta->len++] = ptr;
}

void ptr_array_clear(ptr_array_t *pta) {
    free(pta->data);
    pta->data = NULL;
    pta->len = 0;
}

void ptr_array_remove(ptr_array_t *pta, void *ptr) {
    for (uint i = 0; i < pta->len; i++) {
        if (ptr_array_index(pta, i) == ptr) {
            for (uint j = i; j < pta->len - 1; j++)
                pta->data[j] = pta->data[j + 1];
            pta->len--;
            break;
        }
    }
}

void ptr_array_free(ptr_array_t *pta, bool free_all) {
    if (free_all) {
        for (uint32_t i = 0; i < pta->len; i++)
            free(ptr_array_index(pta, i));
    }
    
    free(pta->data);
    free(pta);
}

table_t* table_new() {
    table_t *tbl = malloc(sizeof(table_t));
    tbl->keys = NULL;
    tbl->vals = NULL;
    tbl->len = 0;
    return tbl;
}

void table_insert(table_t *tbl, const char *key, void *val) {
    if (!tbl->len) {
        tbl->keys = malloc(sizeof(char *));
        tbl->vals = malloc(sizeof(void *));
    } else {
        tbl->keys = realloc(tbl->keys, sizeof(char *) * (tbl->len + 1));
        tbl->vals = realloc(tbl->vals, sizeof(void *) * (tbl->len + 1));
    }
    tbl->keys[tbl->len] = strdup(key);
    tbl->vals[tbl->len] = val;
    tbl->len++;
}

void* table_lookup(table_t *tbl, const char *key) {
    for (uint i = 0; i < tbl->len; i++) {
        if (!strcmp(tbl->keys[i], key))
            return tbl->vals[i];
    }
    return NULL;
}

void table_clear(table_t *tbl, bool free_all) {
    if (free_all) {
        for (uint i = 0; i < tbl->len; i++)
            free(tbl->keys[i]);
    }
    
    free(tbl->keys);
    free(tbl->vals);
    tbl->keys = NULL;
    tbl->vals = NULL;
    tbl->len = 0;
}
