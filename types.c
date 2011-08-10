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
            for (uint j = i; j < pta->len; i++)
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
