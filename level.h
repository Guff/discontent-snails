#ifndef DS_LEVEL_H
#define DS_LEVEL_H

#include "types.h"

typedef struct {
    double x, y;
} slingshot_t;

typedef struct {
    ptr_array_t *obstacles;
    ptr_array_t *enemies;
    slingshot_t slingshot;
} level_t;

typedef enum {
    OBSTACLE_TYPE_BLOCK
} obstacle_type_t;

typedef struct {
    obstacle_type_t type;
    
    double x, y;
    
    double angle;
} obstacle_t;

typedef enum {
    ENEMY_TYPE_NORMAL
} enemy_type_t;

typedef struct {
    enemy_type_t type;
    double x, y;
} enemy_t;

void level_free(level_t *level);
level_t* level_parse(const char *filename);

#endif
