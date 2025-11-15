#include "game.h"

#include <entry.h>

#include "core/ymemory.h"

#include <vulkan_renderer_plugin_main.h>
#include <webgpu_renderer_plugin_main.h>

// Define the function to create a game
b8 create_application(APPLICATION* out_game) {
    // Application configuration.
    out_game->app_config.start_pos_x = 100;
    out_game->app_config.start_pos_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "YWMAA Engine Testbed";
    out_game->boot = game_boot;
    out_game->init = game_init;
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->on_resize = game_on_resize;
    out_game->shutdown = game_shutdown;

    // Create the game state.
    out_game->state_memory_requirement = sizeof(GAME_STATE);
    out_game->state = 0;
    //PRINT_INFO(get_memory_usage_str());

    out_game->engine_state = 0;

    switch (out_game->app_config.renderer_backend_api)
    {
        case RENDERER_BACKEND_API_VULKAN:
            if (!vulkan_renderer_plugin_create(&out_game->render_plugin)) {
                return false;
            }
            break;
        case RENDERER_BACKEND_API_WEBGPU:
            if (!webgpu_renderer_plugin_create(&out_game->render_plugin)) {
                return false;
            }
            break;
        
        default:
            return false;
            break;
    }

    return true;
}