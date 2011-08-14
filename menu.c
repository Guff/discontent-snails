#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>

#include "discontent_snails.h"
#include "level.h"
#include "menu.h"

uint pointer_over_level(uint num_levels, ALLEGRO_EVENT ev) {
    for (uint32_t i = 1; i <= num_levels; i++) {
        float x0, y0, x1, y1;
        x0 = (i - 1) * 200 + 50;
        y0 = 50;
        x1 = x0 + 100;
        y1 = y0 + 100;
        
        if (ev.mouse.x >= x0 && ev.mouse.x <= x1 &&
            ev.mouse.y >= y0 && ev.mouse.y <= y1)
            return i;
    }
    
    return 0;
}

void menu_show(ALLEGRO_DISPLAY *display, ALLEGRO_EVENT_QUEUE *event_queue) {
    al_set_target_backbuffer(display);
    ALLEGRO_BITMAP *bg = al_load_bitmap("data/menu-bg.png");
    ALLEGRO_FONT *font = al_load_ttf_font("data/DejaVuSans.ttf", 12, 0);
    
    ptr_array_t *level_data = level_data_query("levels/");
    
    bool running = true;
    ALLEGRO_EVENT ev;
    
    ALLEGRO_TIMER *frames_timer = al_create_timer(1 / FPS);
    
    al_register_event_source(event_queue, al_get_timer_event_source(frames_timer));
    al_start_timer(frames_timer);
    
    uint selection = 0;
    
    while (running) {
        al_wait_for_event(event_queue, &ev);
        switch (ev.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                running = false;
                break;
            case ALLEGRO_EVENT_TIMER:
                if (ev.timer.source != frames_timer)
                    break;
                al_draw_bitmap(bg, 0, 0, 0);
                for (uint32_t i = 1; i <= level_data->len; i++) {
                    level_data_t *level = ptr_array_index(level_data, i - 1);
                    float x0, y0, x1, y1;
                    x0 = (i - 1) * 200 + 50;
                    y0 = 50;
                    x1 = x0 + 100;
                    y1 = y0 + 100;
                    
                    al_draw_filled_rectangle(x0, y0, x1, y1, al_map_rgba(255, 255, 255, 200));
                    al_draw_text(font, al_map_rgb(0, 0, 0), x0 + 20, y0 + 20, 0, level->name);
                }
                al_flip_display();
                break;
            case ALLEGRO_EVENT_KEY_DOWN:
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_ESCAPE:
                        running = false;
                        break;
                    default:
                        break;
                }
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            {
                uint prev_selection = selection;
                selection = pointer_over_level(level_data->len, ev);
                printf("%i %i\n", prev_selection, selection);
                if (selection && selection == prev_selection) {
                    level_data_t *level_datum = ptr_array_index(level_data,
                                                                selection - 1);
                    level_t *level = level_parse(level_datum->filename);
                    level_play(level, display, event_queue);
                }
                break;
            }
            default:
                break;
        }
    }
}
