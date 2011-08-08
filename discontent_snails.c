#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <chipmunk/chipmunk.h>

#define FPS             30.0
#define PHYSICS_STEP    (1 / 100.0)
#define RECT_MASS       1

cpSpace *space;
static bool running = true;

typedef struct {
    cpBody *body;
    ALLEGRO_BITMAP *bitmap;
} body_t;

body_t rect1, rect2;

void init_world(void) {
    space = cpSpaceNew();
    cpSpaceSetGravity(space, cpv(0, 100));
    cpShape *ground = cpSegmentShapeNew(space->staticBody, cpv(-320, 240),
                                        cpv(320, 240), 0);
    cpShapeSetFriction(ground, 0.1);
    
    cpSpaceAddShape(space, ground);
}

void init_rects(void) {
    cpFloat moment1 = cpMomentForBox(RECT_MASS, 50, 10);
    cpFloat moment2 = cpMomentForBox(RECT_MASS, 10, 50);
    
    rect1.body = cpSpaceAddBody(space, cpBodyNew(RECT_MASS, moment1));
    rect2.body = cpSpaceAddBody(space, cpBodyNew(RECT_MASS, moment2));
    
    cpBodySetPos(rect1.body, cpv(-320, -240));
    cpBodySetPos(rect2.body, cpv(0, -240));
    cpShape *rect1shape = cpBoxShapeNew(rect1.body, 50, 10);
    cpShape *rect2shape = cpBoxShapeNew(rect2.body, 10, 50);
    
    cpShapeSetFriction(rect1shape, 0.1);
    cpShapeSetFriction(rect2shape, 0.1);
    cpSpaceAddShape(space, rect1shape);
    cpSpaceAddShape(space, rect2shape);
    
    rect1.bitmap = al_create_bitmap(50, 10);
    rect2.bitmap = al_create_bitmap(10, 50);

    al_set_target_bitmap(rect1.bitmap);
    al_clear_to_color(al_map_rgb(255, 255, 255));
    
    al_set_target_bitmap(rect2.bitmap);
    al_clear_to_color(al_map_rgb(255, 255, 255));
}

void draw_frame(ALLEGRO_DISPLAY *display) {
    al_set_target_backbuffer(display);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    
    cpVect pos1 = cpBodyGetPos(rect1.body);
    cpFloat ang1 = cpBodyGetAngle(rect1.body);
    cpVect pos2 = cpBodyGetPos(rect2.body);
    cpFloat ang2 = cpBodyGetAngle(rect2.body);

    al_draw_rotated_bitmap(rect1.bitmap, 25, 5, pos1.x + 320, pos1.y + 240, ang1, 0);
    al_draw_rotated_bitmap(rect2.bitmap, 5, 25, pos2.x + 320, pos2.y + 240, ang2, 0);
    
    al_flip_display();
}

int main(int argc, char **argv) {
    al_init();
    
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_MIPMAP);
    
    ALLEGRO_DISPLAY *display = al_create_display(640, 480);
    ALLEGRO_EVENT_QUEUE *event_queue = al_create_event_queue();
    ALLEGRO_EVENT event;
    ALLEGRO_TIMER *frames_timer = al_create_timer(1 / FPS);
    ALLEGRO_TIMER *phys_timer = al_create_timer(PHYSICS_STEP);
    
    al_install_keyboard();
    
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(phys_timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    
    al_start_timer(frames_timer);
    al_start_timer(phys_timer);
    
    init_world();
    init_rects();
    
    while (running) {
        al_wait_for_event(event_queue, &event);
        switch (event.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                running = false;
                break;
            case ALLEGRO_EVENT_TIMER:
                if (event.timer.source == phys_timer)
                    cpSpaceStep(space, PHYSICS_STEP);
                else if (event.timer.source == frames_timer)
                    draw_frame(display);
                break;
            case ALLEGRO_EVENT_KEY_DOWN:
                switch (event.keyboard.keycode) {
                    case ALLEGRO_KEY_ESCAPE:
                        running = false;
                        break;
                    case ALLEGRO_KEY_UP:
                        cpBodySetForce(rect1.body, cpv(0, -5));
                        break;
                    case ALLEGRO_KEY_DOWN:
                        cpBodySetForce(rect1.body, cpv(0, 5));
                        break;
                    case ALLEGRO_KEY_LEFT:
                        cpBodySetForce(rect1.body, cpv(-5, 0));
                        break;
                    case ALLEGRO_KEY_RIGHT:
                        cpBodySetForce(rect1.body, cpv(5, 0));
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    
    return 0;
}
