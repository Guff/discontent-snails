#ifndef DS_MAIN_H
#define DS_MAIN_H

#include <allegro5/allegro.h>
#include "level.h"

#define FPS             30.0

void level_play(level_t *level, ALLEGRO_DISPLAY *display, 
                ALLEGRO_EVENT_QUEUE *event_queue);

#endif
