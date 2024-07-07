#pragma once

#include <defines.h>
#include <game_types.h>

typedef struct GAME_STATE {
    f32 delta_time;
} GAME_STATE;

b8 game_init(GAME* game_instance);

b8 game_update(GAME* game_instance, f32 delta_time);

b8 game_render(GAME* game_instance, f32 delta_time);

void game_on_resize(GAME* game_instance, u32 width, u32 height);