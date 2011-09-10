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
#include "drawing.h"
#include "textures.h"
#include "discontent_snails.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define PHYSICS_STEP    (1 / 250.0)
#define RECT_MASS       2

enum {
    COLLISION_TYPE_SNAIL = 1,
    COLLISION_TYPE_SLINGSHOT,
    COLLISION_TYPE_DESTROYABLE,
    COLLISION_TYPE_STATIC
};

level_state_t state;
table_t *textures;

poof_t* poof_new(cpVect pos) {
    poof_t *poof = calloc(1, sizeof(poof_t));
    poof->timer = al_create_timer(0.5);
    al_register_event_source(state.event_queue, al_get_timer_event_source(poof->timer));
    al_start_timer(poof->timer);
    poof->pos = pos;
    return poof;
}

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
        cpSpaceRemoveShape(state.space, body->shape);
    if (body->body)
        cpSpaceRemoveBody(state.space, body->body);
    body->removed = true;
}

void destroyable_collision_post_step(cpSpace *space, void *obj, void *data) {
    body_t *body = data;
    if (!body || !body->shape || !body->body)
        return;
    body_remove(body);
    if (body->type == BODY_TYPE_ENEMY) {
        state.score += 1000;
        ptr_array_remove(state.enemies, body);
        poof_t *poof = poof_new(cpBodyGetPos(body->body));
        ptr_array_add(state.poofs, poof);
        if (!state.enemies->len)
            state.victorious = true;
    } else if (body->type == BODY_TYPE_OBSTACLE) {
        state.score += 200;
        ptr_array_remove(state.obstacles, body);
        poof_t *poof = poof_new(cpBodyGetPos(body->body));
        ptr_array_add(state.poofs, poof);
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
    cpSpaceRemoveConstraint(state.space, state.spring);
    cpConstraintDestroy(state.spring);
    state.spring = NULL;
}

cpBool slingshot_collision_pre_solve(cpArbiter *arb, cpSpace *space, void *data) {
    if (state.spring) {
        cpArbiterIgnore(arb);
        cpSpaceAddPostStepCallback(state.space, slingshot_collision_post_step,
                                   state.spring, NULL);
        return cpFalse;
    }
    return cpTrue;
}

void world_init(level_t *level) {
    state.space = cpSpaceNew();
    
    state.slingshot = body_new();
    state.ground = body_new();
    
    cpSpaceSetGravity(state.space, cpv(0, 200));
    state.ground->type = BODY_TYPE_GROUND;
    state.ground->shape = cpSegmentShapeNew(state.space->staticBody,
                                            cpv(-100, HEIGHT - 40),
                                            cpv(WIDTH + 100, HEIGHT - 40), 0);
    
    cpShapeSetCollisionType(state.ground->shape, COLLISION_TYPE_STATIC);
    cpShapeSetUserData(state.ground->shape, state.ground);
    cpShapeSetElasticity(state.ground->shape, 0.3);
    cpShapeSetFriction(state.ground->shape, 0.8);
    
    cpSpaceAddShape(state.space, state.ground->shape);

    state.slingshot->body = cpBodyNewStatic();
    state.slingshot->shape = cpSegmentShapeNew(state.slingshot->body, cpv(0, 0),
                                               cpv(0, -80), 0);
    state.slingshot->bitmap = al_create_bitmap(40, 80);
    
    cpBodySetPos(state.slingshot->body, cpv(level->slingshot.x, level->slingshot.y));
    cpSpaceAddShape(state.space, state.slingshot->shape);
    cpSpaceReindexStatic(state.space);
    
    state.ground->bitmap = table_lookup(textures, "ground");
    
    al_set_target_bitmap(state.slingshot->bitmap);
    al_draw_scaled_bitmap(table_lookup(textures, "slingshot"), 0, 0, 160, 320, 0,
                          0, 40, 80, 0);
    
    state.terrain_bitmap = al_create_bitmap(2 * WIDTH, 2 * HEIGHT);
    terrain_draw(level->terrain, state.terrain_bitmap);
}

void snails_init(ptr_array_t *snails_data) {
    cpVect snail_normal_verts[] = { cpv(-3, 14), cpv(13, 6), cpv(14, 0),
                                    cpv(12, -8), cpv(6, -13), cpv(0, -14),
                                    cpv(-9, -11), cpv(-12, -4), cpv(-13, 1),
                                    cpv(-9, 10) };
    int snail_normal_verts_n = sizeof(snail_normal_verts) / sizeof(cpVect);
    
    for (uint32_t i = 0; i < snails_data->len; i++) {
        cpFloat moment = cpMomentForPoly(1, snail_normal_verts_n,
                                         snail_normal_verts,
                                         cpvzero);
        
        snail_t *snail = ptr_array_index(snails_data, i);
        body_t *body = body_new();
        body->type = BODY_TYPE_SNAIL;
        body->body = cpSpaceAddBody(state.space, cpBodyNew(1, moment));
        cpBodySetPos(body->body, cpv(snail->x, snail->y));
        
        body->shape = cpPolyShapeNew(body->body,
                                     snail_normal_verts_n,
                                     snail_normal_verts,
                                     cpvzero);
        cpShapeSetUserData(body->shape, body);
        cpShapeSetElasticity(body->shape, 0.85);
        
        cpShapeSetFriction(body->shape, 0.6);
        cpSpaceAddShape(state.space, body->shape);
        
        body->bitmap = al_create_bitmap(30, 30);
        al_set_target_bitmap(body->bitmap);
        al_draw_scaled_bitmap(table_lookup(textures, "snail-normal"), 0, 0, 120,
                          120, 0, 0, 30, 30, 0);    

        cpShapeSetCollisionType(body->shape, COLLISION_TYPE_SNAIL);
        
        ptr_array_add(state.snails, body);
    }
}

void obstacles_init(ptr_array_t *obstacles_data) {
    // initialize obstacles
    for (uint32_t i = 0; i < obstacles_data->len; i++) {
        cpFloat moment = cpMomentForBox(RECT_MASS, 50, 10);
        obstacle_t *obstacle = ptr_array_index(obstacles_data, i);
        body_t *body = body_new();
        body->type = BODY_TYPE_OBSTACLE;
        body->body = cpSpaceAddBody(state.space, cpBodyNew(RECT_MASS, moment));
        cpBodySetPos(body->body, cpv(obstacle->x, obstacle->y));
        cpBodySetAngle(body->body, obstacle->angle / 180 * M_PI);
        
        body->shape = cpBoxShapeNew(body->body, 50, 10);
        cpShapeSetUserData(body->shape, body);
        cpShapeSetCollisionType(body->shape, COLLISION_TYPE_DESTROYABLE);
        cpShapeSetElasticity(body->shape, 0.15);
        cpShapeSetFriction(body->shape, 0.5);
        cpSpaceAddShape(state.space, body->shape);
        
        body->bitmap = al_create_bitmap(50, 10);
        
        body->data = obstacle;
        
        ALLEGRO_BITMAP *texture;
        
        switch (obstacle->type) {
            case OBSTACLE_TYPE_WOOD:
                texture = table_lookup(textures, "wooden-block-50x10");
                break;
            case OBSTACLE_TYPE_STONE:
                texture = table_lookup(textures, "stone-block-50x10");
                break;
            case OBSTACLE_TYPE_GLASS:
                texture = table_lookup(textures, "glass-block-50x10");
                break;
            default:
                break;
        }
        
        al_set_target_bitmap(body->bitmap);
        al_draw_scaled_bitmap(texture, 0, 0, 200, 40, 0, 0, 50, 10, 0);
        
        ptr_array_add(state.obstacles, body);
    }
}

void enemies_init(ptr_array_t *enemies_data) {
    for (uint32_t i = 0; i < enemies_data->len; i++) {
        cpFloat moment = cpMomentForBox(1.5, 30, 30);
        enemy_t *enemy = ptr_array_index(enemies_data, i);
        body_t *body = body_new();
        body->type = BODY_TYPE_ENEMY;
        body->body = cpSpaceAddBody(state.space, cpBodyNew(1.5, moment));
        cpBodySetPos(body->body, cpv(enemy->x, enemy->y));
        
        body->shape = cpBoxShapeNew(body->body, 30, 30);
        cpShapeSetUserData(body->shape, body);
        cpShapeSetCollisionType(body->shape, COLLISION_TYPE_DESTROYABLE);
        cpShapeSetElasticity(body->shape, 0.45);
        cpShapeSetFriction(body->shape, 0.8);
        cpSpaceAddShape(state.space, body->shape);
        
        body->bitmap = al_create_bitmap(30, 30);
        
        al_set_target_bitmap(body->bitmap);
        al_clear_to_color(al_map_rgb(255, 255, 255));
        
        ptr_array_add(state.enemies, body);
    }
}

void bodies_init(level_t *level) {
    state.snails = ptr_array_new();
    state.launched_snails = ptr_array_new();
    state.obstacles = ptr_array_new();
    state.enemies = ptr_array_new();
    state.poofs = ptr_array_new();
    
    cpShapeSetCollisionType(state.slingshot->shape, COLLISION_TYPE_SLINGSHOT);
    
    cpSpaceAddCollisionHandler(state.space, COLLISION_TYPE_SNAIL,
                               COLLISION_TYPE_SLINGSHOT, NULL,
                               slingshot_collision_pre_solve, NULL, NULL, NULL);
    cpSpaceAddCollisionHandler(state.space, COLLISION_TYPE_SNAIL,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    cpSpaceAddCollisionHandler(state.space, COLLISION_TYPE_SLINGSHOT,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    cpSpaceAddCollisionHandler(state.space, COLLISION_TYPE_DESTROYABLE,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    cpSpaceAddCollisionHandler(state.space, COLLISION_TYPE_STATIC,
                               COLLISION_TYPE_DESTROYABLE, NULL, NULL,
                               destroyable_collision_post_solve, NULL, NULL);
    
    // initialize snails
    snails_init(level->snails);
    state.spring = cpDampedSpringNew(((body_t *)ptr_array_index(state.snails, 0))->body,
                                     state.slingshot->body, cpvzero, cpv(0, -80),
                                     1, 10, 0);
    cpSpaceAddConstraint(state.space, state.spring);
    
    obstacles_init(level->obstacles);
    
    enemies_init(level->enemies);
}

void physics_step(void) {
    cpSpaceStep(state.space, PHYSICS_STEP);
    
    for (uint i = 0; i < state.snails->len; i++) {
        body_t *snail = ptr_array_index(state.snails, i);
        if (!snail->removed && cpBodyIsSleeping(snail->body)) {
            poof_t *poof = poof_new(cpBodyGetPos(snail->body));
            ptr_array_add(state.poofs, poof);
            body_remove(snail);
        }
    }
}

void poofs_handle(poof_t *poof) {
    al_stop_timer(poof->timer);
    al_unregister_event_source(state.event_queue,
                               al_get_timer_event_source(poof->timer));
    al_destroy_timer(poof->timer);
    ptr_array_remove(state.poofs, poof);
    free(poof);
}

cpConstraint* mouse_spring_add(cpBody *snail_body, cpBody *mouse_body) {
    return cpDampedSpringNew(mouse_body, snail_body, cpv(0, 0), cpv(0, 0),
                             1, 25, 0);
}

void level_play(level_t *level, ALLEGRO_DISPLAY *display, 
                ALLEGRO_EVENT_QUEUE *event_queue) {
    struct {
        bool ctrl;
        bool shift;
    } keys = { false, false };
    
    state.victorious = false;
    bool running = true;
    bool pressed = false;
    state.view.zoom = 1;
    state.view.pan_x = 0;
    state.view.pan_y = 0;
    state.score = 0;
    
    ALLEGRO_EVENT ev;
    
    world_init(level);
    bodies_init(level);
    
    cpSpaceSetSleepTimeThreshold(state.space, 5.0);
    
    ALLEGRO_TIMER *frames_timer = al_create_timer(1 / FPS);
    ALLEGRO_TIMER *phys_timer = al_create_timer(PHYSICS_STEP);
    
    al_register_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(phys_timer));
    
    al_start_timer(frames_timer);
    al_start_timer(phys_timer);
    
    state.scene = al_create_bitmap(al_get_display_width(display) * 3,
                             al_get_display_height(display) * 3);
    
    al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE);
    
    cpBody *mouse_body = cpBodyNew(INFINITY, INFINITY);
    
    body_t *snail = ptr_array_index(state.snails, state.launched_snails->len);
    cpVect pos = cpBodyGetPos(snail->body);
    cpBodySetPos(mouse_body, cpv(pos.x - 25, pos.y - 10));
    cpConstraint *mouse_spring = mouse_spring_add(snail->body, mouse_body);
    cpSpaceAddConstraint(state.space, mouse_spring);
    
    while (running) {
        al_wait_for_event(event_queue, &ev);
        switch (ev.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                running = false;
                break;
            case ALLEGRO_EVENT_TIMER:
            {
                // poofs are all of the same duration, and added in order,
                // so the first one is always the next to be removed
                poof_t *poof;
                if (state.poofs->len)
                    poof = ptr_array_index(state.poofs, 0);
                if (ev.timer.source == phys_timer) {
                    physics_step();
                } else if (ev.timer.source == frames_timer) {
                    draw_frame(display, state.scene, state);
                } else if (state.poofs->len && ev.timer.source == poof->timer) {
                    poofs_handle(poof);
                }
                break;
            }
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
                        state.view.zoom = 1;
                        state.view.pan_x = 0;
                        state.view.pan_y = 0;
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
                if (pressed && cpSpaceContainsConstraint(state.space, mouse_spring)) {
                    cpSpaceRemoveConstraint(state.space, mouse_spring);
                    cpConstraintFree(mouse_spring);
                    ptr_array_add(state.launched_snails, snail);
                    snail = ptr_array_index(state.snails, state.launched_snails->len);
                    if (snail) {
                        mouse_spring = mouse_spring_add(snail->body, mouse_body);
                        cpSpaceAddConstraint(state.space, mouse_spring);
                    }
                }
                pressed = false;
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                if (ev.mouse.dz && keys.ctrl)
                    state.view.zoom = MAX(MIN(3, state.view.zoom - ev.mouse.dz / 10.0),
                                          1 / 3.0);
                if (ev.mouse.dz)
                    state.view.pan_y = MIN(MAX(0, state.view.pan_y + ev.mouse.dz * 10),
                                     HEIGHT);
                if (ev.mouse.dw)
                    state.view.pan_x = MAX(0, state.view.pan_x + ev.mouse.dw * 10);
                if (pressed) {
                    cpBodySetPos(mouse_body, cpv(ev.mouse.x, ev.mouse.y));
                }
                break;
            default:
                break;
        }
        
        if (state.victorious)
            running = false;
    }
    
    al_unregister_event_source(state.event_queue,
                               al_get_timer_event_source(frames_timer));
    al_unregister_event_source(state.event_queue,
                               al_get_timer_event_source(phys_timer));
    
    for (uint i = 0; i < state.obstacles->len; i++)
        body_free(ptr_array_index(state.obstacles, i), true);
    
    for (uint i = 0; i < state.enemies->len; i++)
        body_free(ptr_array_index(state.enemies, i), true);
    
    body_free(state.ground, false);
    body_free(state.slingshot, true);
    
    ptr_array_free(state.obstacles, false);
    ptr_array_free(state.enemies, false);
    
    al_destroy_bitmap(state.scene);
    
    cpSpaceFree(state.space);
    ALLEGRO_TRANSFORM trans;
    al_identity_transform(&trans);
    al_use_transform(&trans);
    al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
    
    level_free(level);
}

int main(int argc, char **argv) {
    al_init();
    
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
    
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    
    ALLEGRO_DISPLAY *display = al_create_display(WIDTH, HEIGHT);
    al_set_window_title(display, "discontent snails");
    al_set_app_name("discontent-snails");
    state.event_queue = al_create_event_queue();
    
    al_init_image_addon();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    
    state.font = al_load_font("data/DejaVuSans.ttf", 12, 0);
    
    al_install_keyboard();
    al_install_mouse();
    
    al_register_event_source(state.event_queue, al_get_display_event_source(display));
    al_register_event_source(state.event_queue, al_get_keyboard_event_source());
    al_register_event_source(state.event_queue, al_get_mouse_event_source());

    textures = textures_load();
    
    menu_show(display, state.event_queue);
    
    al_shutdown_image_addon();
    
    return 0;
}
