#ifndef DS_TYPES_H
#define DS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t len;
    void **data;
} ptr_array_t;

typedef struct {
    uint32_t len;
    size_t size;
    void *data;
} dyn_array_t;

typedef struct {
    uint32_t len;
    char **keys;
    void **vals;
} table_t;

ptr_array_t* ptr_array_new();
void* ptr_array_index(ptr_array_t *pta, uint32_t n);
void ptr_array_add(ptr_array_t *pta, void *ptr);
void ptr_array_clear(ptr_array_t *pta);
void ptr_array_remove(ptr_array_t *pta, void *ptr);
void ptr_array_free(ptr_array_t *pta, bool free_all);

table_t* table_new();
void table_insert(table_t *tbl, const char *key, void *val);
void* table_lookup(table_t *tbl, const char *key);
void table_clear(table_t *tbl, bool free_all);

#endif
