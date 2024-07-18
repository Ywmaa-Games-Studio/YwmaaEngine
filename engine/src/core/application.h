#pragma once

#include "defines.h"

struct GAME;

// Application configuration.
typedef struct APPLICATION_CONFIG {
    // Window starting position x axis, if applicable.
    i16 start_pos_x;

    // Window starting position y axis, if applicable.
    i16 start_pos_y;

    // Window starting width, if applicable.
    i16 start_width;

    // Window starting height, if applicable.
    i16 start_height;

    // The application name used in windowing, if applicable.
    char* name;
} APPLICATION_CONFIG;


YAPI b8 application_create(struct GAME* game_instance);

YAPI b8 application_run();


void application_get_framebuffer_size(u32* width, u32* height);