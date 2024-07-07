#include "game.h"

#include <core/logger.h>

b8 game_init(GAME* game_instance) {
    PRINT_DEBUG("game_init() called!");
    return TRUE;
}

b8 game_update(GAME* game_instance, f32 delta_time) {
    // PRINT_DEBUG("game_update() called!");
    return TRUE;
}

b8 game_render(GAME* game_instance, f32 delta_time) {
    return TRUE;
}

void game_on_resize(GAME* game_instance, u32 width, u32 height) {
}