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

struct {
    cpFloat pan_x, pan_y;
    cpFloat zoom;
} view;

cpSpace *space;
cpConstraint *spring;
ALLEGRO_BITMAP *scene;
ALLEGRO_BITMAP *terrain_bitmap;
ALLEGRO_FONT *font;
uint32_t score;
body_t *slingshot, *ground;
table_t *textures;
ptr_array_t *snails;
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
        score += 1000;
        if (!enemies->len)
            victorious = true;
    } else if (body->type == BODY_TYPE_OBSTACLE) {
        score += 200;
        ptr_array_remove(obstacles, body);
    }
    
    //body_free(body, true);
}

void destroyable_collision_post_solve(cpArbiter *arb, cpSpace *space, void *data) {
    CP_ARBITER_GET_SHAPES(arb, a, b);
    CP_ARBITER_GET_BODIES(arb, b0, b1);
    
    body_t *body = cpShapeGetUserData(a);
    if (!body || body->type != BODY_TYPE_OBSTACLE || body->type != BODY_TYPE_ENEMY)
        body = cpShapeGetUserData(b);
    cpVect impulse = cpArbiterTotalImpulseWithFriction(arb);
    body->damage += MAX(cpvlength(impulse) - 250, 0) / 20;
    
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

void terrain_draw(ptr_array_t *terrain, ALLEGRO_BITMAP *bitmap) {
    al_set_target_bitmap(bitmap);
    ALLEGRO_VERTEX v[terrain->len * 3 + 1];
    for (uint i = 0; i < terrain->len; i++) {
        triangle_t *tri = ptr_array_index(terrain, i);
        v[3 * i].x = tri->x0 * 2;
        v[3 * i].y = tri->y0 * 2;
        v[3 * i].color = al_map_rgb(255, 255, 255);
        v[3 * i].u = tri->x0 * 2;
        v[3 * i].v = tri->y0 * 2;
        
        v[3 * i + 1].x = tri->x1 * 2;
        v[3 * i + 1].y = tri->y1 * 2;
        v[3 * i + 1].color = al_map_rgb(255, 255, 255);
        v[3 * i + 1].u = tri->x1 * 2;
        v[3 * i + 1].v = tri->y1 * 2;
        
        v[3 * i + 2].x = tri->x2 * 2;
        v[3 * i + 2].y = tri->y2 * 2;
        v[3 * i + 2].color = al_map_rgb(255, 255, 255);
        v[3 * i + 2].u = tri->x2 * 2;
        v[3 * i + 2].v = tri->y2 * 2;
        
        al_draw_triangle(tri->x0 * 2, tri->y0 * 2, tri->x1 * 2, tri->y1 * 2,
                         tri->x2 * 2, tri->y2 * 2, al_map_rgb(20, 30, 70), 4);
        
        cpVect cp_tri[3] = { cpv(tri->x0, tri->y0), cpv(tri->x1, tri->y1),
                             cpv(tri->x2, tri->y2) };
        cpShape *tri_shape = cpPolyShapeNew(space->staticBody, 3, cp_tri,
                                            cpv(0, 0));
        cpSpaceAddShape(space, tri_shape);
    }
    al_draw_prim(v, NULL, table_lookup(textures, "stone"), 0,
                 3 * terrain->len, ALLEGRO_PRIM_TRIANGLE_LIST);
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
                                         cpv(0, -80), 0);
    slingshot->bitmap = al_create_bitmap(40, 80);
    
    cpBodySetPos(slingshot->body, cpv(level->slingshot.x, level->slingshot.y));
    cpSpaceAddShape(space, slingshot->shape);
    cpSpaceReindexStatic(space);
    
    ground->bitmap = table_lookup(textures, "ground");
    
    al_set_target_bitmap(slingshot->bitmap);
    al_draw_scaled_bitmap(table_lookup(textures, "slingshot"), 0, 0, 160, 320, 0,
                          0, 40, 80, 0);
    
    terrain_bitmap = al_create_bitmap(2 * WIDTH, 2 * HEIGHT);
    terrain_draw(level->terrain, terrain_bitmap);
}

void init_bodies(level_t *level) {
    snails = ptr_array_new();
    obstacles = ptr_array_new();
    enemies = ptr_array_new();
    
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
    
    cpVect snail_normal_verts[] = { cpv(-3, 14), cpv(13, 6), cpv(14, 0),
                                    cpv(12, -8), cpv(6, -13), cpv(0, -14),
                                    cpv(-9, -11), cpv(-12, -4), cpv(-13, 1),
                                    cpv(-9, 10) };
    int snail_normal_verts_n = sizeof(snail_normal_verts) / sizeof(cpVect);
    
    // initialize snails
    for (uint32_t i = 0; i < level->snails->len; i++) {
        cpFloat moment = cpMomentForPoly(1, snail_normal_verts_n,
                                         snail_normal_verts,
                                         cpvzero);
        
        snail_t *snail = ptr_array_index(level->snails, i);
        body_t *body = body_new();
        body->type = BODY_TYPE_SNAIL;
        body->body = cpSpaceAddBody(space, cpBodyNew(1, moment));
        cpBodySetPos(body->body, cpv(snail->x, snail->y));
        
        body->shape = cpPolyShapeNew(body->body,
                                     snail_normal_verts_n,
                                     snail_normal_verts,
                                     cpvzero);
        cpShapeSetUserData(body->shape, body);
        cpShapeSetElasticity(body->shape, 0.85);
        
        cpShapeSetFriction(body->shape, 0.6);
        cpSpaceAddShape(space, body->shape);
        
        body->bitmap = al_create_bitmap(30, 30);
        al_set_target_bitmap(body->bitmap);
        al_draw_scaled_bitmap(table_lookup(textures, "snail-normal"), 0, 0, 120,
                          120, 0, 0, 30, 30, 0);    

        cpShapeSetCollisionType(body->shape, COLLISION_TYPE_SNAIL);
        
        ptr_array_add(snails, body);
    }
    
    spring = cpDampedSpringNew(((body_t *)ptr_array_index(snails, 0))->body,
                               slingshot->body, cpvzero, cpv(0, -80), 1, 10, 0);
    cpSpaceAddConstraint(space, spring);
    
    // initialize obstacles
    for (uint32_t i = 0; i < level->obstacles->len; i++) {
        cpFloat moment = cpMomentForBox(RECT_MASS, 50, 10);
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
    
    // initialize enemies
    for (uint32_t i = 0; i < level->enemies->len; i++) {
        cpFloat moment = cpMomentForBox(1.5, 30, 30);
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
    al_draw_scaled_bitmap(terrain_bitmap, 0, 0, 2 * WIDTH, 2 * HEIGHT, 0, 0,
                          WIDTH, HEIGHT, 0);
                          
    cpVect sling_pos = cpBodyGetPos(slingshot->body);
    al_draw_bitmap(slingshot->bitmap, sling_pos.x - 20, sling_pos.y - 80, 0);
    
    for (uint32_t i = 0; i < snails->len; i++) {
        body_t *body = ptr_array_index(snails, i);
            
        cpVect pos = cpBodyGetPos(body->body);
        cpFloat angle = cpBodyGetAngle(body->body);
        
        al_draw_rotated_bitmap(body->bitmap, 15, 15, pos.x, pos.y,
                               angle, 0);
    }
    
    for (uint32_t i = 0; i < obstacles->len; i++) {
        body_t *body = ptr_array_index(obstacles, i);
        cpVect pos = cpBodyGetPos(body->body);
        cpFloat angle = cpBodyGetAngle(body->body);
        al_draw_rotated_bitmap(body->bitmap, 25, 5, pos.x, pos.y, angle, 0);
    }
    
    for (uint32_t i = 0; i < enemies->len; i++) {
        body_t *body = ptr_array_index(enemies, i);
        cpVect pos = cpBodyGetPos(body->body);
        cpFloat angle = cpBodyGetAngle(body->body);
        al_draw_rotated_bitmap(body->bitmap, 15, 15, pos.x, pos.y, angle, 0);
    }
    

    al_set_target_backbuffer(display);
    al_identity_transform(&trans);
    al_translate_transform(&trans, -view.pan_x, -2 * (HEIGHT - view.pan_y) * view.zoom);
    al_scale_transform(&trans, 1 / view.zoom, 1 / view.zoom);
    al_use_transform(&trans);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(scene, 0, 0, 0);
    
    al_identity_transform(&trans);
    al_use_transform(&trans);
    char score_text[11];
    snprintf(score_text, 11, "%u", score);
    al_draw_text(font, al_map_rgb(255, 255, 255), 0, 0, 0,
                 score_text);
    
    al_flip_display();
}

void level_play(level_t *level, ALLEGRO_DISPLAY *display, 
                ALLEGRO_EVENT_QUEUE *event_queue) {
    struct {
        bool ctrl;
        bool shift;
    } keys = { false, false };
    
    victorious = false;
    bool running = true;
    bool pressed = false;
    view.zoom = 1;
    view.pan_x = 0;
    view.pan_y = 0;
    score = 0;
    
    ALLEGRO_EVENT ev;
    
    init_world(level);
    init_bodies(level);
    
    cpSpaceSetSleepTimeThreshold(space, 5.0);
    
    ALLEGRO_TIMER *frames_timer = al_create_timer(1 / FPS);
    ALLEGRO_TIMER *phys_timer = al_create_timer(PHYSICS_STEP);
    
    al_register_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(phys_timer));
    
    al_start_timer(frames_timer);
    al_start_timer(phys_timer);
    
    scene = al_create_bitmap(al_get_display_width(display) * 3,
                             al_get_display_height(display) * 3);
    
    al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE);
    
    cpBody *mouse_body = cpBodyNew(INFINITY, INFINITY);
    body_t *snail = ptr_array_index(snails, 0);
    cpVect pos = cpBodyGetPos(snail->body);
    cpBodySetPos(mouse_body, cpv(pos.x - 25, pos.y - 10));
    cpConstraint *mouse_spring = cpDampedSpringNew(mouse_body, snail->body,
                                                   cpv(0, 0), cpv(0, 0),
                                                   1, 25, 0.5);
    cpSpaceAddConstraint(space, mouse_spring);
    
    while (running) {
        al_wait_for_event(event_queue, &ev);
        switch (ev.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                running = false;
                break;
            case ALLEGRO_EVENT_TIMER:
                if (ev.timer.source == phys_timer) {
                    cpSpaceStep(space, PHYSICS_STEP);
                    if (cpBodyIsSleeping(snail->body)) {
                        cpSpaceRemoveShape(space, snail->shape);
                        cpSpaceRemoveBody(space, snail->body);
                        ptr_array_remove(snails, snail);
                    }
                }
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
                        view.zoom = 1;
                        view.pan_x = 0;
                        view.pan_y = 0;
                        break;
                    case ALLEGRO_KEY_LCTRL:
                    case ALLEGRO_KEY_RCTRL:
                        keys.ctrl = true;
                        break;
                    case ALLEGRO_KEY_LSHIFT:
                    case ALLEGRO_KEY_RSHIFT:
                        keys.shift = true;
                        break;
                    default:
                        break;
                }
                break;
            case ALLEGRO_EVENT_KEY_UP:
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_LCTRL:
                    case ALLEGRO_KEY_RCTRL:
                        keys.ctrl = false;
                        break;
                    case ALLEGRO_KEY_LSHIFT:
                    case ALLEGRO_KEY_RSHIFT:
                        keys.shift = false;
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
                if (pressed && mouse_spring) {
                    cpSpaceRemoveConstraint(space, mouse_spring);
                    mouse_spring = NULL;
                }
                pressed = false;
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                if (ev.mouse.dz && keys.ctrl)
                    view.zoom = MAX(MIN(3, view.zoom - ev.mouse.dz / 10.0), 1 / 3.0);
                if (ev.mouse.dz)
                    view.pan_y += ev.mouse.dz * 10;
                if (ev.mouse.dw)
                    view.pan_x += ev.mouse.dw * 10;
                if (pressed) {
                    cpBodySetPos(mouse_body, cpv(ev.mouse.x, ev.mouse.y));
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
    
    font = al_load_font("data/DejaVuSans.ttf", 12, 0);
    
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
