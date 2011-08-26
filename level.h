#ifndef DS_LEVEL_H
#define DS_LEVEL_H

#include "types.h"

typedef struct {
    double x0, y0, x1, y1, x2, y2;
} triangle_t;

typedef struct {
    double x, y;
} slingshot_t;

typedef struct {
    const char *name;
    uint16_t num;
    const char *filename;
} level_data_t;

typedef struct {
    double width, height;
    ptr_array_t *snails;
    ptr_array_t *obstacles;
    ptr_array_t *enemies;
    slingshot_t slingshot;
    ptr_array_t *terrain;
} level_t;

typedef enum {
    SNAIL_TYPE_NORMAL
} snail_type_t;

typedef struct {
    snail_type_t type;
    
    double x, y;
} snail_t;

typedef enum {
    OBSTACLE_SHAPE_RECT,
    OBSTACLE_SHAPE_CIRCLE,
    OBSTACLE_SHAPE_TRIANGLE
} obstacle_shape_t;

typedef enum {
    OBSTACLE_TYPE_WOOD,
    OBSTACLE_TYPE_STONE,
    OBSTACLE_TYPE_GLASS
} obstacle_type_t;

typedef struct {
    obstacle_shape_t shape;
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

ptr_array_t* level_data_query(const char *path);

void level_free(level_t *level);
level_t* level_parse(const char *filename);

#endif
