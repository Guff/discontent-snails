#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <jansson.h>

#include "level.h"

#define LVL_FSIZE       8192

void level_free(level_t *level) {
    ptr_array_free(level->obstacles, true);
    free(level);
}

level_t *level_parse(const char *filename) {
    level_t *level = malloc(sizeof(level_t));
    json_error_t error;
    json_t *root = json_load_file(filename, 0, &error);
    bool parse_error = false;
    
    if (!root) {
        fprintf(stderr, "error on line %d: %s\n", error.line, error.text);
        parse_error = true;
        goto root_lbl;
    }
    
    json_t *obstacles = json_object_get(root, "obstacles");
    
    if (!json_is_array(obstacles)) {
        fprintf(stderr, "\"obstacles\" is not an array\n");
        parse_error = true;
        goto obstacles_lbl;
    }
    
    level->obstacles = ptr_array_new();
    
    for (size_t i = 0; i < json_array_size(obstacles); i++) {
        json_t *obstacle_obj = json_array_get(obstacles, i);
        
        if (!json_is_object(obstacle_obj)) {
            fprintf(stderr, "obstacle is not an object\n");
            parse_error = true;
            goto obstacle_lbl;
        }
        
        json_t *type_obj, *x_obj, *y_obj, *angle_obj;
        const char *type_str;
        obstacle_type_t type;
        double x, y, angle;
        
        type_obj = json_object_get(obstacle_obj, "type");
        if (!json_is_string(type_obj)) {
            fprintf(stderr, "\"type\" is not a string\n");
            parse_error = true;
            goto type_lbl;
        }
        
        type_str = json_string_value(type_obj);
        
        if (!strcmp(type_str, "block")) {
            type = OBSTACLE_TYPE_BLOCK;
        } else {
            fprintf(stderr, "\"type\" not a valid obstacle type\n");
            parse_error = true;
            goto type_lbl;
        }
        
        x_obj = json_object_get(obstacle_obj, "x");
        if (!json_is_number(x_obj)) {
            fprintf(stderr, "\"x\" is not a number\n");
            parse_error = true;
            goto x_lbl;
        }

        x = json_number_value(x_obj);
        
        y_obj = json_object_get(obstacle_obj, "y");
        if (!json_is_number(y_obj)) {
            fprintf(stderr, "\"y\" is not a number\n");
            parse_error = true;
            goto y_lbl;
        }
        
        y = json_number_value(y_obj);
        
        angle_obj = json_object_get(obstacle_obj, "angle");
        if (!json_is_number(angle_obj)) {
            fprintf(stderr, "\"angle\" is not a number\n");
            parse_error = true;
            goto angle_lbl;
        }
        
        angle = json_number_value(angle_obj);
        
        obstacle_t *obstacle = malloc(sizeof(obstacle_t));
        obstacle->type  = type;
        obstacle->x     = x;
        obstacle->y     = y;
        obstacle->angle = angle;
        
        ptr_array_add(level->obstacles, obstacle);
        
angle_lbl:
        json_decref(angle_obj);
y_lbl:
        json_decref(y_obj);
x_lbl:
        json_decref(x_obj);
type_lbl:
        json_decref(type_obj);
obstacle_lbl:
        json_decref(obstacle_obj);
    }
    
obstacles_lbl:
    json_decref(obstacles);
root_lbl:
    json_decref(root);

    if (parse_error) {
        level_free(level);
        level = NULL;
    }
    
    return level;
        
}
