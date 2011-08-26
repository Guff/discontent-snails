#include <stdio.h>
#include <string.h>
#include <allegro5/allegro.h>

#include "textures.h"

const char * const texture_files[] = { "wooden-block-50x10",
                                       "wooden-block-50x10-damage1",
                                       "stone-block-50x10",
                                       "glass-block-50x10",
                                       "snail-normal",
                                       "ground", "menu-bg", "sky-bg",
                                       "slingshot", "stone", "poof" };

table_t *textures_load(void) {
    table_t *textures = table_new();
    
    for (uint32_t i = 0; i < sizeof(texture_files) / sizeof(char*); i++) {
        char filename[strlen(texture_files[i]) + 5];
        strcpy(filename, "data/");
        strcat(filename, texture_files[i]);
        strcat(filename, ".png");
        table_insert(textures, texture_files[i], al_load_bitmap(filename));
    }
    
    return textures;
}
