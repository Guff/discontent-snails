#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <jansson.h>
#include <allegro5/allegro.h>

#include "level.h"

static level_data_t* level_data_new(void) {
    return calloc(1, sizeof(level_data_t));
}

ptr_array_t* level_data_query(const char *path) {
    ptr_array_t *level_data = ptr_array_new();
    ALLEGRO_FS_ENTRY *dir = al_create_fs_entry(path);
    al_open_directory(dir);
    ALLEGRO_FS_ENTRY *entry;
    while ((entry = al_read_directory(dir))) {
        level_data_t *level = level_data_new();
        level->filename = al_get_fs_entry_name(entry);
        
        bool parse_error = false;
        json_error_t error;
        json_t *root = json_load_file(level->filename, 0, &error);
        
        if (!root) {
            fprintf(stderr, "error on line %d: %s\n", error.line, error.text);
            parse_error = true;
            goto root_lbl;
        }
        
        json_t *name_obj = json_object_get(root, "name");
        if (!json_is_string(name_obj)) {
            fprintf(stderr, "\"name\" is not a string\n");
            parse_error = true;
            goto root_lbl;
        }
        
        level->name = strdup(json_string_value(name_obj));
        
        json_t *num_obj = json_object_get(root, "num");
        if (!json_is_integer(num_obj)) {
            fprintf(stderr, "\"name\" is not an integer\n");
            parse_error = true;
            goto root_lbl;
        }
        
        level->num = json_integer_value(num_obj);
root_lbl:
        json_decref(root);
        
        if (parse_error) {
            ptr_array_free(level_data, true);
            level_data = NULL;
            break;
        } else {
            ptr_array_add(level_data, level);
        }
    }
    
    al_close_directory(dir);
    
    return level_data;
}

void level_free(level_t *level) {
    ptr_array_free(level->obstacles, true);
    ptr_array_free(level->enemies, true);
    ptr_array_free(level->terrain, true);
    free(level);
}

level_t* level_parse(const char *filename) {
    level_t *level = malloc(sizeof(level_t));
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    bool parse_error = false;
    
    level->obstacles = ptr_array_new();
    level->enemies = ptr_array_new();
    level->terrain = ptr_array_new();
    
    if (!root) {
        fprintf(stderr, "error on line %d: %s\n", error.line, error.text);
        parse_error = true;
        goto root_lbl;
    }
    
    json_t *obstacles = json_object_get(root, "obstacles");
    
    if (!json_is_array(obstacles)) {
        fprintf(stderr, "\"obstacles\" is not an array\n");
        parse_error = true;
        goto root_lbl;
    }
    
    for (size_t i = 0; i < json_array_size(obstacles); i++) {
        json_t *obstacle_obj = json_array_get(obstacles, i);
        
        if (!json_is_object(obstacle_obj)) {
            fprintf(stderr, "obstacle is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        json_t *type_obj, *x_obj, *y_obj, *angle_obj;
        const char *type_str;
        obstacle_type_t type;
        double x, y, angle;
        
        type_obj = json_object_get(obstacle_obj, "type");
        if (!json_is_string(type_obj)) {
            fprintf(stderr, "\"type\" is not a string\n");
            parse_error = true;
            goto root_lbl;
        }
        
        type_str = json_string_value(type_obj);
        
        if (!strcmp(type_str, "block")) {
            type = OBSTACLE_TYPE_BLOCK;
        } else {
            fprintf(stderr, "\"type\" not a valid obstacle type\n");
            parse_error = true;
            goto root_lbl;
        }
        
        x_obj = json_object_get(obstacle_obj, "x");
        if (!json_is_number(x_obj)) {
            fprintf(stderr, "\"x\" is not a number\n");
            parse_error = true;
            goto root_lbl;
        }

        x = json_number_value(x_obj);
        
        y_obj = json_object_get(obstacle_obj, "y");
        if (!json_is_number(y_obj)) {
            fprintf(stderr, "\"y\" is not a number\n");
            parse_error = true;
            goto root_lbl;
        }
        
        y = json_number_value(y_obj);
        
        angle_obj = json_object_get(obstacle_obj, "angle");
        if (!json_is_number(angle_obj)) {
            fprintf(stderr, "\"angle\" is not a number\n");
            parse_error = true;
            goto root_lbl;
        }
        
        angle = json_number_value(angle_obj);
        
        obstacle_t *obstacle = malloc(sizeof(obstacle_t));
        obstacle->type  = type;
        obstacle->x     = x;
        obstacle->y     = y;
        obstacle->angle = angle;
        
        ptr_array_add(level->obstacles, obstacle);
    }
    
    json_t *slingshot = json_object_get(root, "slingshot");
    if (!json_is_object(slingshot)) {
        fprintf(stderr, "\"slingshot\" is not an object\n");
        parse_error = true;
        goto root_lbl;
    }
    
    json_t *slingshot_x = json_object_get(slingshot, "x");
    if (!json_is_number(slingshot_x)) {
        fprintf(stderr, "\"x\" is not a number\n");
        parse_error = true;
        goto root_lbl;
    }
    
    level->slingshot.x = json_number_value(slingshot_x);
    
    json_t *slingshot_y = json_object_get(slingshot, "y");
    if (!json_is_number(slingshot_y)) {
        fprintf(stderr, "\"y\" is not a number\n");
        parse_error = true;
        goto root_lbl;
    }
    
    level->slingshot.y = json_number_value(slingshot_y);
    
    json_t *enemies = json_object_get(root, "enemies");
    if (!json_is_array(enemies)) {
        fprintf(stderr, "\"enemies\" is not an array\n");
        parse_error = true;
        goto root_lbl;
    }
    
    for (size_t i = 0; i < json_array_size(enemies); i++) {
        json_t *enemy_obj = json_array_get(enemies, i);
        
        if (!json_is_object(enemy_obj)) {
            fprintf(stderr, "enemy is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        json_t *type_obj, *x_obj, *y_obj;
        const char *type_str;
        enemy_type_t type;
        double x, y;
        
        type_obj = json_object_get(enemy_obj, "type");
        if (!json_is_string(type_obj)) {
            fprintf(stderr, "\"type\" is not a string\n");
            parse_error = true;
            goto root_lbl;
        }
        
        type_str = json_string_value(type_obj);
        
        if (!strcmp(type_str, "normal")) {
            type = ENEMY_TYPE_NORMAL;
        } else {
            fprintf(stderr, "\"type\" not a valid enemy type\n");
            parse_error = true;
            goto root_lbl;
        }
        
        x_obj = json_object_get(enemy_obj, "x");
        if (!json_is_number(x_obj)) {
            fprintf(stderr, "\"x\" is not a number\n");
            parse_error = true;
            goto root_lbl;
        }

        x = json_number_value(x_obj);
        
        y_obj = json_object_get(enemy_obj, "y");
        if (!json_is_number(y_obj)) {
            fprintf(stderr, "\"y\" is not a number\n");
            parse_error = true;
            goto root_lbl;
        }
        
        y = json_number_value(y_obj);
        
        enemy_t *enemy = malloc(sizeof(enemy_t));
        enemy->type  = type;
        enemy->x     = x;
        enemy->y     = y;
        
        ptr_array_add(level->enemies, enemy);
    }
    
    json_t *terrain = json_object_get(root, "terrain");
    if (!json_is_array(terrain)) {
        fprintf(stderr, "\"terrain\" is not an array\n");
        parse_error = true;
        goto root_lbl;
    }
    
    for (size_t i = 0; i < json_array_size(terrain); i++) {
        json_t *tri_obj = json_array_get(terrain, i);
        if (!json_is_object(tri_obj)) {
            fprintf(stderr, "triangle is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        triangle_t *tri = malloc(sizeof(triangle_t));
        
        json_t *x_obj = json_object_get(tri_obj, "x0");
        if (!json_is_number(x_obj)) {
            fprintf(stderr, "x0 is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        tri->x0 = json_number_value(x_obj);
        
        json_t *y_obj = json_object_get(tri_obj, "y0");
        if (!json_is_number(y_obj)) {
            fprintf(stderr, "y0 is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        tri->y0 = json_number_value(y_obj);
        
        x_obj = json_object_get(tri_obj, "x1");
        if (!json_is_number(x_obj)) {
            fprintf(stderr, "x1 is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        tri->x1 = json_number_value(x_obj);
        
        y_obj = json_object_get(tri_obj, "y1");
        if (!json_is_number(y_obj)) {
            fprintf(stderr, "y1 is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        tri->y1 = json_number_value(y_obj);
        
        x_obj = json_object_get(tri_obj, "x2");
        if (!json_is_number(x_obj)) {
            fprintf(stderr, "x2 is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        tri->x2 = json_number_value(x_obj);
        
        y_obj = json_object_get(tri_obj, "y2");
        if (!json_is_number(y_obj)) {
            fprintf(stderr, "y2 is not an object\n");
            parse_error = true;
            goto root_lbl;
        }
        
        tri->y2 = json_number_value(y_obj);
        
        ptr_array_add(level->terrain, tri);
    }
    
root_lbl:
    json_decref(root);
    if (parse_error) {
        level_free(level);
        level = NULL;
    }
    
    return level;
        
}
