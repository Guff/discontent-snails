#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t len;
    void **data;
} ptr_array_t;

#define ptr_array_index(pta, n) (pta)->data[n]
ptr_array_t* ptr_array_new();
void ptr_array_add(ptr_array_t *pta, void *ptr);
void ptr_array_clear(ptr_array_t *pta);
void ptr_array_remove(ptr_array_t *pta, void *ptr);
void ptr_array_free(ptr_array_t *pta, bool free_all);
