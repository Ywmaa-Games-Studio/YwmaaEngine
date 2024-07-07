#include "game.h"

#include <entry.h>

#include "core/ymemory.h"

// Define the function to create a game
b8 create_game(GAME* out_game) {
    // Application configuration.
    out_game->app_config.start_pos_x = 100;
    out_game->app_config.start_pos_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "YWMAA Engine Testbed";
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->init = game_init;
    out_game->on_resize = game_on_resize;

    // Create the game state.
    out_game->state = yallocate(sizeof(GAME_STATE), MEMORY_TAG_GAME);
    //PRINT_INFO(get_memory_usage_str());

    return TRUE;
}