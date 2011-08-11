#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <chipmunk/chipmunk.h>

#include "level.h"
#include "textures.h"

#define FPS             30.0
#define PHYSICS_STEP    (1 / 250.0)
#define RECT_MASS       1
#define WIDTH           640
#define HEIGHT          480

cpSpace *space;

static bool running = true;
static bool pressed = false;

typedef struct {
    cpBody *body;
    cpShape *shape;
    ALLEGRO_BITMAP *bitmap;
} body_t;

ALLEGRO_BITMAP *snail_bitmap;
ALLEGRO_BITMAP *bg_bitmap;
ALLEGRO_BITMAP *ground_bitmap;
ALLEGRO_BITMAP *slingshot_bitmap;

cpConstraint *spring;
body_t *snail, *slingshot;
ptr_array_t *obstacles;

body_t* body_new(void) {
    return calloc(1, sizeof(body_t));
}

void slingshot_collision_post_step(cpSpace *space, void *obj, void *data) {
    cpSpaceRemoveConstraint(space, spring);
    cpConstraintDestroy(spring);
    spring = NULL;
}

cpBool slingshot_collision_pre_solve(cpArbiter *arb, cpSpace *space, void *data) {
    if (spring) {
        cpArbiterIgnore(arb);
        cpSpaceAddPostStepCallback(space, slingshot_collision_post_step, spring,
                                   NULL);
        return cpFalse;
    }
    return cpTrue;
}

void init_world(level_t *level) {
    space = cpSpaceNew();
    
    slingshot = body_new();
    
    cpSpaceSetGravity(space, cpv(0, 200));
    cpShape *ground = cpSegmentShapeNew(space->staticBody, cpv(-100, HEIGHT - 40),
                                        cpv(WIDTH + 100, HEIGHT - 40), 0);
    cpShapeSetElasticity(ground, 0.3);
    cpShapeSetFriction(ground, 0.8);
    
    cpSpaceAddShape(space, ground);

    slingshot->body = cpBodyNewStatic();
    slingshot->shape = cpSegmentShapeNew(slingshot->body, cpv(0, 0),
                                         cpv(0, -40), 0);
    slingshot->bitmap = al_create_bitmap(13, 40);
    
    cpBodySetPos(slingshot->body, cpv(level->slingshot.x, level->slingshot.y));
    cpSpaceAddShape(space, slingshot->shape);
    cpSpaceReindexStatic(space);
    
    bg_bitmap = al_load_bitmap("data/sky-bg.png");
    ground_bitmap = al_load_bitmap("data/ground.png");
    slingshot_bitmap = al_load_bitmap("data/slingshot.png");
    
    al_set_target_bitmap(slingshot->bitmap);
    al_draw_scaled_bitmap(slingshot_bitmap, 0, 0, 40, 120, 0, 0, 13, 40, 0);
}

void init_bodies(level_t *level) {
    table_t *textures = textures_load();
    
    snail = body_new();
    obstacles = ptr_array_new();
    
    cpVect snail_verts[] = { cpv(-3, 14), cpv(13, 6), cpv(14, 0), cpv(12, -8),
                             cpv(6, -13), cpv(0, -14), cpv(-9, -11), cpv(-12, -4),
                             cpv(-13, 1), cpv(-9, 10) };
    int snail_verts_n = sizeof(snail_verts) / sizeof(cpVect);
    
    cpFloat moment = cpMomentForPoly(RECT_MASS, snail_verts_n, snail_verts,
                                     cpv(0, 0));
    
    snail->body = cpSpaceAddBody(space, cpBodyNew(RECT_MASS, moment));
    
    cpBodySetPos(snail->body, cpv(0, 435));
    snail->shape = cpPolyShapeNew(snail->body, 
                                  sizeof(snail_verts) / sizeof(cpVect),
                                  snail_verts,
                                  cpv(0, 0));
    
    cpShapeSetElasticity(snail->shape, 0.65);
    
    cpShapeSetFriction(snail->shape, 0.6);
    cpSpaceAddShape(space, snail->shape);
    
    snail->bitmap = al_create_bitmap(30, 30);
    
    spring = cpDampedSpringNew(snail->body, slingshot->body, cpv(0, 0),
                               cpv(0, -40), 50, 10, 0);
    cpSpaceAddConstraint(space, spring);

    al_set_target_bitmap(snail->bitmap);
    al_draw_scaled_bitmap(snail_bitmap, 0, 0, 120, 120, 0, 0, 30, 30, 0);
    
    cpShapeSetCollisionType(snail->shape, 1);
    cpShapeSetCollisionType(slingshot->shape, 2);
    
    cpSpaceAddCollisionHandler(space, 1, 2, NULL, slingshot_collision_pre_solve,
                               NULL, NULL, NULL);
    
    for (uint32_t i = 0; i < level->obstacles->len; i++) {
        moment = cpMomentForBox(RECT_MASS, 40, 10);
        obstacle_t *obstacle = ptr_array_index(level->obstacles, i);
        body_t *body = body_new();
        body->body = cpSpaceAddBody(space, cpBodyNew(RECT_MASS, moment));
        cpBodySetPos(body->body, cpv(obstacle->x, obstacle->y));
        cpBodySetAngle(body->body, obstacle->angle / 180 * M_PI);
        
        body->shape = cpBoxShapeNew(body->body, 40, 10);
        cpShapeSetElasticity(body->shape, 0.65);
        cpShapeSetFriction(body->shape, 0.8);
        cpSpaceAddShape(space, body->shape);
        
        body->bitmap = al_create_bitmap(40, 10);
        
        ALLEGRO_BITMAP *texture;
        
        switch (obstacle->type) {
            case OBSTACLE_TYPE_BLOCK:
                texture = table_lookup(textures, texture_files[0]);
                break;
            default:
                break;
        }
        
        al_set_target_bitmap(body->bitmap);
        al_draw_scaled_bitmap(texture, 0, 0, 100, 25, 0, 0, 40, 10, 0);
        
        ptr_array_add(obstacles, body);
    }
}

void draw_frame(ALLEGRO_DISPLAY *display) {
    al_set_target_backbuffer(display);
    al_draw_bitmap(bg_bitmap, 0, 0, 0);
    
    al_draw_bitmap(ground_bitmap, 0, HEIGHT - 50, 0);
    
    cpVect snail_pos = cpBodyGetPos(snail->body);
    cpFloat snail_ang = cpBodyGetAngle(snail->body);
    
    al_draw_rotated_bitmap(snail->bitmap, 15, 15, snail_pos.x, snail_pos.y,
                           snail_ang, 0);
    
    cpVect sling_pos = cpBodyGetPos(slingshot->body);
    al_draw_bitmap(slingshot->bitmap, sling_pos.x - 7, sling_pos.y - 40, 0);
    
    for (uint32_t i = 0; i < obstacles->len; i++) {
        body_t *body = ptr_array_index(obstacles, i);
        cpVect pos = cpBodyGetPos(body->body);
        cpFloat angle = cpBodyGetAngle(body->body);
        al_draw_rotated_bitmap(body->bitmap, 20, 5, pos.x, pos.y, angle, 0);
    }
    
    al_flip_display();
}

int main(int argc, char **argv) {
    al_init();
    
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_MIPMAP);
    
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    
    ALLEGRO_DISPLAY *display = al_create_display(WIDTH, HEIGHT);
    ALLEGRO_EVENT_QUEUE *event_queue = al_create_event_queue();
    ALLEGRO_EVENT ev;
    ALLEGRO_TIMER *frames_timer = al_create_timer(1 / FPS);
    ALLEGRO_TIMER *phys_timer = al_create_timer(PHYSICS_STEP);
    
    al_init_image_addon();
    al_install_keyboard();
    al_install_mouse();
    
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(phys_timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());
    
    al_start_timer(frames_timer);
    al_start_timer(phys_timer);
    
    snail_bitmap = al_load_bitmap("data/snail-normal.png");
    
    level_t *level = level_parse("level.json");
    
    init_world(level);
    init_bodies(level);
    
    while (running) {
        al_wait_for_event(event_queue, &ev);
        switch (ev.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                running = false;
                break;
            case ALLEGRO_EVENT_TIMER:
                if (ev.timer.source == phys_timer)
                    cpSpaceStep(space, PHYSICS_STEP);
                else if (ev.timer.source == frames_timer)
                    draw_frame(display);
                break;
            case ALLEGRO_EVENT_KEY_DOWN:
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_ESCAPE:
                        running = false;
                        break;
                    case ALLEGRO_KEY_UP:
                        cpBodyApplyImpulse(snail->body, cpv(0, -75), cpv(0, -10));
                        break;
                    case ALLEGRO_KEY_DOWN:
                        cpBodyApplyImpulse(snail->body, cpv(0, 75), cpv(0, -10));
                        break;
                    case ALLEGRO_KEY_LEFT:
                        cpBodyApplyImpulse(snail->body, cpv(-75, 0), cpv(0, -10));
                        break;
                    case ALLEGRO_KEY_RIGHT:
                        cpBodyApplyImpulse(snail->body, cpv(75, 0), cpv(0, -10));
                        break;
                    default:
                        break;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                {
                cpVect pos = cpv(ev.mouse.x, ev.mouse.y);
                
                if (cpShapePointQuery(snail->shape, pos))
                    pressed = true;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                pressed = false;
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                if (pressed)
                    cpBodySetPos(snail->body, cpv(ev.mouse.x, ev.mouse.y));
                break;
            default:
                break;
        }
    }
    
    al_shutdown_image_addon();
    
    return 0;
}
