#ifndef DS_MAIN_H
#define DS_MAIN_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <chipmunk/chipmunk.h>

#include "level.h"

#define FPS             30.0
#define WIDTH           640
#define HEIGHT          480

typedef struct {
    cpFloat pan_x, pan_y;
    cpFloat zoom;
} view_t;

typedef enum {
    BODY_TYPE_SNAIL,
    BODY_TYPE_SLINGSHOT,
    BODY_TYPE_OBSTACLE,
    BODY_TYPE_ENEMY,
    BODY_TYPE_GROUND
} body_type_t;

typedef struct {
    body_type_t type;
    bool removed;
    cpBody *body;
    cpShape *shape;
    ALLEGRO_BITMAP *bitmap;
    cpFloat damage;
    void *data;
} body_t;

typedef struct {
    ALLEGRO_TIMER *timer;
    cpVect pos;
} poof_t;

typedef struct {
    cpSpace *space;
    cpConstraint *spring;
    ALLEGRO_BITMAP *scene;
    ALLEGRO_BITMAP *terrain_bitmap;
    ALLEGRO_FONT *font;
    ALLEGRO_EVENT_QUEUE *event_queue;
    uint32_t score;
    view_t view;
    body_t *slingshot, *ground;
    ptr_array_t *snails;
    ptr_array_t *launched_snails;
    ptr_array_t *obstacles;
    ptr_array_t *enemies;
    ptr_array_t *poofs;
    bool victorious;
} level_state_t;

extern table_t *textures;
extern level_state_t state;

void level_play(level_t *level, ALLEGRO_DISPLAY *display, 
                ALLEGRO_EVENT_QUEUE *event_queue);

#endif
