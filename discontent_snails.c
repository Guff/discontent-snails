#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <chipmunk/chipmunk.h>

#include "menu.h"
#include "textures.h"
#include "discontent_snails.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define PHYSICS_STEP    (1 / 250.0)
#define RECT_MASS       2
#define WIDTH           640
#define HEIGHT          480

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
} body_t;

enum {
    COLLISION_TYPE_SNAIL = 1,
    COLLISION_TYPE_SLINGSHOT,
    COLLISION_TYPE_DESTROYABLE,
    COLLISION_TYPE_STATIC
};

cpSpace *space;
cpConstraint *spring;
cpFloat zoom, pan_x;
ALLEGRO_BITMAP *scene;
body_t *snail, *slingshot, *ground;
table_t *textures;
ptr_array_t *obstacles;
ptr_array_t *enemies;
bool victorious;

body_t* body_new(void) {
    return calloc(1, sizeof(body_t));
}

void body_free(body_t *body, bool free_bitmap) {
    if (body->shape)
        cpShapeFree(body->shape);
    if (body->body)
        cpBodyFree(body->body);
    // some of the bitmaps are handled by textures_load(); don't free those
    if (free_bitmap) {
        if (body->bitmap)
            al_destroy_bitmap(body->bitmap);
    }
    free(body);
}

void body_remove(body_t *body) {
    if (!body || body->removed)
        return;
    if (body->shape)
        cpSpaceRemoveShape(space, body->shape);
    if (body->body)
        cpSpaceRemoveBody(space, body->body);
    body->removed = true;
}

void destroyable_collision_post_step(cpSpace *space, void *obj, void *data) {
    body_t *body = data;
    if (!body || !body->shape || !body->body)
        return;
    body_remove(body);
    if (body->type == BODY_TYPE_ENEMY) {
        ptr_array_remove(enemies, body);
        if (!enemies->len)
            victorious = true;
    } else if (body->type == BODY_TYPE_OBSTACLE)
        ptr_array_remove(obstacles, body);
    
    //body_free(body, true);
}

void destroyable_collision_post_solve(cpArbiter *arb, cpSpace *space, void *data) {
    CP_ARBITER_GET_SHAPES(arb, a, b);
    CP_ARBITER_GET_BODIES(arb, b0, b1);
    
    body_t *body = cpShapeGetUserData(a);
    if (!body || body->type != BODY_TYPE_OBSTACLE || body->type != BODY_TYPE_ENEMY)
        body = cpShapeGetUserData(b);
    cpVect impulse = cpArbiterTotalImpulseWithFriction(arb);
    body->damage += MAX(cpvlength(impulse) - 250, 0) / 200;
    
    if (body->damage >= 1) {
        cpSpaceAddPostStepCallback(space, destroyable_collision_post_step, arb,
                                   body);
    }
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
    ground = body_new();
    
    cpSpaceSetGravity(space, cpv(0, 200));
    ground->type = BODY_TYPE_GROUND;
    ground->shape = cpSegmentShapeNew(space->staticBody,
                                           cpv(-100, HEIGHT - 40),
                                           cpv(WIDTH + 100, HEIGHT - 40), 0);
    
    cpShapeSetCollisionType(ground->shape, COLLISION_TYPE_STATIC);
    cpShapeSetUserData(ground->shape, ground);
    cpShapeSetElasticity(ground->shape, 0.3);
    cpShapeSetFriction(ground->shape, 0.8);
    
    cpSpaceAddShape(space, ground->shape);

    slingshot->body = cpBodyNewStatic();
    slingshot->shape = cpSegmentShapeNew(slingshot->body, cpv(0, 0),
                                         cpv(0, -40), 0);
    slingshot->bitmap = al_create_bitmap(13, 40);
    
    cpBodySetPos(slingshot->body, cpv(level->slingshot.x, level->slingshot.y));
    cpSpaceAddShape(space, slingshot->shape);
    cpSpaceReindexStatic(space);
    
    ground->bitmap = table_lookup(textures, "ground");
    
    al_set_target_bitmap(slingshot->bitmap);
    al_draw_scaled_bitmap(table_lookup(textures, "slingshot"), 0, 0, 40, 120, 0,
                          0, 13, 40, 0);
}

void init_bodies(level_t *level) {
    snail = body_new();
    obstacles = ptr_array_new();
    enemies = ptr_array_new();
    
    cpVect snail_verts[] = { cpv(-3, 14), cpv(13, 6), cpv(14, 0), cpv(12, -8),
                             cpv(6, -13), cpv(0, -14), cpv(-9, -11), cpv(-12, -4),
                             cpv(-13, 1), cpv(-9, 10) };
    int snail_verts_n = sizeof(snail_verts) / sizeof(cpVect);
    
    cpFloat moment = cpMomentForPoly(1, snail_verts_n, snail_verts,
                                     cpv(0, 0));
    
    snail->body = cpSpaceAddBody(space, cpBodyNew(1, moment));
    
    cpBodySetPos(snail->body, cpv(70, 435));
    snail->shape = cpPolyShapeNew(snail->body, 
                                  sizeof(snail_verts) / sizeof(cpVect),
                                  snail_verts,
                                  cpv(0, 0));
    cpShapeSetUserData(snail->shape, snail);
    cpShapeSetElasticity(snail->shape, 0.85);
    
    cpShapeSetFriction(snail->shape, 0.6);
    cpSpaceAddShape(space, snail->shape);
    
    snail->bitmap = al_create_bitmap(30, 30);
    
    spring = cpDampedSpringNew(snail->body, slingshot->body, cpv(0, 0),
                               cpv(0, -40), 50, 10, 0);
    cpSpaceAddConstraint(space, spring);

    al_set_target_bitmap(snail->bitmap);
    al_draw_scaled_bitmap(table_lookup(textures, "snail-normal"), 0, 0, 120,
                          120, 0, 0, 30, 30, 0);
    
    cpShapeSetCollisionType(snail->shape, COLLISION_TYPE_SNAIL);
    cpShapeSetCollisionType(slingshot->shape, COLLISION_TYPE_SLINGSHOT);
    
    cpSpaceAddCollisionHandler(space, COLLISION_TYPE_SNAIL,
                               COLLISION_TYPE_SLINGSHOT, NULL,
                               slingshot_collision_pre_solve, NULL, NULL, NULL);
    cpSpaceAddCollisionHandler(space, COLLISION_TYPE_SNAIL,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    cpSpaceAddCollisionHandler(space, COLLISION_TYPE_SLINGSHOT,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    cpSpaceAddCollisionHandler(space, COLLISION_TYPE_DESTROYABLE,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    cpSpaceAddCollisionHandler(space, COLLISION_TYPE_STATIC,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    
    for (uint32_t i = 0; i < level->obstacles->len; i++) {
        moment = cpMomentForBox(RECT_MASS, 50, 10);
        obstacle_t *obstacle = ptr_array_index(level->obstacles, i);
        body_t *body = body_new();
        body->type = BODY_TYPE_OBSTACLE;
        body->body = cpSpaceAddBody(space, cpBodyNew(RECT_MASS, moment));
        cpBodySetPos(body->body, cpv(obstacle->x, obstacle->y));
        cpBodySetAngle(body->body, obstacle->angle / 180 * M_PI);
        
        body->shape = cpBoxShapeNew(body->body, 50, 10);
        cpShapeSetUserData(body->shape, body);
        cpShapeSetCollisionType(body->shape, COLLISION_TYPE_DESTROYABLE);
        cpShapeSetElasticity(body->shape, 0.15);
        cpShapeSetFriction(body->shape, 0.5);
        cpSpaceAddShape(space, body->shape);
        
        body->bitmap = al_create_bitmap(50, 10);
        
        ALLEGRO_BITMAP *texture;
        
        switch (obstacle->type) {
            case OBSTACLE_TYPE_BLOCK:
                texture = table_lookup(textures, "wooden-block-50x10");
                break;
            default:
                break;
        }
        
        al_set_target_bitmap(body->bitmap);
        al_draw_scaled_bitmap(texture, 0, 0, 200, 40, 0, 0, 50, 10, 0);
        
        ptr_array_add(obstacles, body);
    }
    
    for (uint32_t i = 0; i < level->enemies->len; i++) {
        moment = cpMomentForBox(1.5, 30, 30);
        enemy_t *enemy = ptr_array_index(level->enemies, i);
        body_t *body = body_new();
        body->type = BODY_TYPE_ENEMY;
        body->body = cpSpaceAddBody(space, cpBodyNew(1.5, moment));
        cpBodySetPos(body->body, cpv(enemy->x, enemy->y));
        
        body->shape = cpBoxShapeNew(body->body, 30, 30);
        cpShapeSetUserData(body->shape, body);
        cpShapeSetCollisionType(body->shape, COLLISION_TYPE_DESTROYABLE);
        cpShapeSetElasticity(body->shape, 0.45);
        cpShapeSetFriction(body->shape, 0.8);
        cpSpaceAddShape(space, body->shape);
        
        body->bitmap = al_create_bitmap(30, 30);
        
        al_set_target_bitmap(body->bitmap);
        al_clear_to_color(al_map_rgb(255, 255, 255));
        
        ptr_array_add(enemies, body);
    }
}

void draw_frame(ALLEGRO_DISPLAY *display) {
    al_set_target_bitmap(scene);
    al_draw_bitmap(table_lookup(textures, "sky-bg"), 0, 0, 0);
    
    ALLEGRO_TRANSFORM trans;
    al_identity_transform(&trans);
    al_translate_transform(&trans, 0, 1440 - HEIGHT);
    al_use_transform(&trans);
    
    al_draw_bitmap(ground->bitmap, 0, HEIGHT - 50, 0);
    
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
        al_draw_rotated_bitmap(body->bitmap, 25, 5, pos.x, pos.y, angle, 0);
        //printf("%f\n", body->damage);
    }
    
    for (uint32_t i = 0; i < enemies->len; i++) {
        body_t *body = ptr_array_index(enemies, i);
        cpVect pos = cpBodyGetPos(body->body);
        cpFloat angle = cpBodyGetAngle(body->body);
        al_draw_rotated_bitmap(body->bitmap, 15, 15, pos.x, pos.y, angle, 0);
    }
    
    al_set_target_backbuffer(display);
    al_identity_transform(&trans);
    al_translate_transform(&trans, -pan_x, HEIGHT * zoom - 1440);
    al_scale_transform(&trans, 1 / zoom, 1 / zoom);
    al_use_transform(&trans);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(scene, 0, 0, 0);
    
    al_flip_display();
}

void level_play(level_t *level, ALLEGRO_DISPLAY *display, 
                ALLEGRO_EVENT_QUEUE *event_queue) {
    victorious = false;
    bool running = true;
    bool pressed = false;
    zoom = 1;
    pan_x = 0;
    ALLEGRO_EVENT ev;
    
    init_world(level);
    init_bodies(level);
    
    ALLEGRO_TIMER *frames_timer = al_create_timer(1 / FPS);
    ALLEGRO_TIMER *phys_timer = al_create_timer(PHYSICS_STEP);
    
    al_register_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(phys_timer));
    
    al_start_timer(frames_timer);
    al_start_timer(phys_timer);
    
    scene = al_create_bitmap(al_get_display_width(display) * 3,
                             al_get_display_height(display) * 3);
    
    al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE);
    
    cpBody *mouse_body = cpBodyNewStatic();
    //cpSpaceAddBody(space, mouse_body);
    cpVect pos = cpBodyGetPos(snail->body);
    cpBodySetPos(mouse_body, cpv(pos.x - 25, pos.y - 10));
    cpConstraint *mouse_spring = cpDampedSpringNew(mouse_body, snail->body,
                                                   cpv(0, 0), cpv(0, 0),
                                                   100, 25, 0);
    cpSpaceAddConstraint(space, mouse_spring);
    
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
                    case ALLEGRO_KEY_1:
                        zoom = 1;
                        pan_x = 1;
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
                if (mouse_spring) {
                    cpSpaceRemoveConstraint(space, mouse_spring);
                    mouse_spring = NULL;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                if (ev.mouse.dz)
                    zoom = MAX(MIN(3, zoom - ev.mouse.dz / 5.0), 1.0 / 3.0);
                if (ev.mouse.dw)
                    pan_x += ev.mouse.dw * 10;
                if (pressed) {
                    cpBodySetPos(mouse_body, cpv(ev.mouse.x, ev.mouse.y));
                    printf("%f\n", cpBodyGetPos(mouse_body).x);
                }
                break;
            default:
                break;
        }
        
        if (victorious)
            running = false;
    }
    
    al_unregister_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_unregister_event_source(event_queue, al_get_timer_event_source(phys_timer));
    
    for (uint i = 0; i < obstacles->len; i++)
        body_free(ptr_array_index(obstacles, i), true);
    
    for (uint i = 0; i < enemies->len; i++)
        body_free(ptr_array_index(enemies, i), true);
    
    body_free(ground, false);
    body_free(snail, true);
    body_free(slingshot, true);
    
    ptr_array_free(obstacles, false);
    ptr_array_free(enemies, false);
    
    al_destroy_bitmap(scene);
    
    cpSpaceFree(space);
    ALLEGRO_TRANSFORM trans;
    al_identity_transform(&trans);
    al_use_transform(&trans);
    al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
}

int main(int argc, char **argv) {
    al_init();
    
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
    
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    
    ALLEGRO_DISPLAY *display = al_create_display(WIDTH, HEIGHT);
    al_set_window_title(display, "discontent snails");
    al_set_app_name("discontent-snails");
    ALLEGRO_EVENT_QUEUE *event_queue = al_create_event_queue();
    
    al_init_image_addon();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    
    al_install_keyboard();
    al_install_mouse();
    
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    textures = textures_load();
    
    menu_show(display, event_queue);
    
    al_shutdown_image_addon();
    
    return 0;
}
