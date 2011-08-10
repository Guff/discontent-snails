#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <chipmunk/chipmunk.h>

#include "level.h"

#define FPS             30.0
#define PHYSICS_STEP    (1 / 100.0)
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

body_t *rect, *slingshot;
ptr_array_t *obstacles;

body_t* body_new(void) {
    return calloc(1, sizeof(body_t));
}

cpVect al_to_cp(cpFloat x, cpFloat y) {
    return cpv(x - WIDTH / 2, y - HEIGHT / 2);
}

void cp_to_al(cpVect vect, cpFloat *x, cpFloat *y) {
    *x = vect.x + WIDTH / 2;
    *y = vect.y + HEIGHT / 2;
}

void init_world(void) {
    space = cpSpaceNew();
    
    slingshot = body_new();
    
    cpSpaceSetGravity(space, cpv(0, 100));
    cpShape *ground = cpSegmentShapeNew(space->staticBody, cpv(-400, 240),
                                        cpv(400, 240), 0);
    cpShapeSetElasticity(ground, 0.3);
    cpShapeSetFriction(ground, 0.8);
    
    cpSpaceAddShape(space, ground);

    slingshot->shape = cpSegmentShapeNew(space->staticBody, cpv(-300, 240),
                                        cpv(-300, 200), 0);
    slingshot->bitmap = al_create_bitmap(2, 40);
    
    cpSpaceAddShape(space, slingshot->shape);
    cpSpaceReindexStatic(space);
}

void init_bodies(void) {
    rect = body_new();
    obstacles = ptr_array_new();
    
    cpFloat moment = cpMomentForBox(RECT_MASS, 50, 10);
    
    rect->body = cpSpaceAddBody(space, cpBodyNew(RECT_MASS, moment));
    
    cpBodySetPos(rect->body, al_to_cp(50, 0));
    rect->shape = cpBoxShapeNew(rect->body, 50, 10);
    
    cpShapeSetElasticity(rect->shape, 0.65);
    
    cpShapeSetFriction(rect->shape, 0.8);
    cpSpaceAddShape(space, rect->shape);
    
    rect->bitmap = al_create_bitmap(50, 10);

    al_set_target_bitmap(rect->bitmap);
    al_clear_to_color(al_map_rgb(255, 255, 255));
    
    al_set_target_bitmap(slingshot->bitmap);
    al_clear_to_color(al_map_rgb(255, 255, 255));
    
    level_t *level = level_parse("level.json");
    for (uint32_t i = 0; i < level->obstacles->len; i++) {
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
        
        ALLEGRO_BITMAP *texture = al_load_bitmap("data/wooden-block-40x10.png");
        
        al_set_target_bitmap(body->bitmap);
        al_draw_bitmap(texture, 0, 0, 0);
        
        al_free(texture);
        
        ptr_array_add(obstacles, body);
    }
}

void draw_frame(ALLEGRO_DISPLAY *display) {
    al_set_target_backbuffer(display);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    
    cpVect rect_pos = cpBodyGetPos(rect->body);
    cpFloat rect_ang = cpBodyGetAngle(rect->body);
    
    cpFloat rect_x, rect_y;
    cp_to_al(rect_pos, &rect_x, &rect_y);

    al_draw_rotated_bitmap(rect->bitmap, 25, 5, rect_x, rect_y, rect_ang, 0);
    
    al_draw_bitmap(slingshot->bitmap, 20, 440, 0);
    
    for (uint32_t i = 0; i < obstacles->len; i++) {
        body_t *body = ptr_array_index(obstacles, i);
        cpVect pos = cpBodyGetPos(body->body);
        cpFloat angle = cpBodyGetAngle(body->body);
        cpFloat x, y;
        cp_to_al(pos, &x, &y);
        al_draw_rotated_bitmap(body->bitmap, 20, 5, x, y, angle, 0);
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
    
    init_world();
    init_bodies();
    
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
                        cpBodyApplyImpulse(rect->body, cpv(0, -75), cpv(0, 0));
                        break;
                    case ALLEGRO_KEY_DOWN:
                        cpBodyApplyImpulse(rect->body, cpv(0, 75), cpv(0, 0));
                        break;
                    case ALLEGRO_KEY_LEFT:
                        cpBodyApplyImpulse(rect->body, cpv(-75, 0), cpv(0, 0));
                        break;
                    case ALLEGRO_KEY_RIGHT:
                        cpBodyApplyImpulse(rect->body, cpv(75, 0), cpv(0, 0));
                        break;
                    default:
                        break;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                {
                cpVect pos = al_to_cp(ev.mouse.x, ev.mouse.y);
                
                if (cpShapePointQuery(rect->shape, pos))
                    pressed = true;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                pressed = false;
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                if (pressed)
                    cpBodySetPos(rect->body, cpv(ev.mouse.x - WIDTH / 2, 
                                                 ev.mouse.y - HEIGHT / 2));
                break;
            default:
                break;
        }
    }
    
    al_shutdown_image_addon();
    
    return 0;
}
