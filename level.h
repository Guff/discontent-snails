#include "types.h"

typedef struct {
    ptr_array_t *obstacles;
} level_t;

typedef enum {
    OBSTACLE_TYPE_BLOCK
} obstacle_type_t;

typedef struct {
    obstacle_type_t type;
    
    double x;
    double y;
    
    double angle;
} obstacle_t;

void level_free(level_t *level);
level_t* level_parse(const char *filename);
