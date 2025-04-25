#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "renderer/renderer_types.inl"
#include "game_types.h"

// Externally-defined function to create a game.
extern b8 create_game(GAME* out_game);
void run_benchmarks();

/**
 * The main entry point of the application in C.
 * This is actually called from the main function in main.zig
 */
int engine_main(E_RENDERER_BACKEND_API renderer_backend_api) {
    // I use this to quickly test the memory system.
/*     run_benchmarks();
    return 0; */
    
    // Request the game instance from the application.
    GAME game_instance;
    game_instance.app_config.renderer_backend_api = renderer_backend_api;
    if (!create_game(&game_instance)) {
        PRINT_ERROR("Could not create game!");
        return -1;
    }

    // Ensure the function pointers exist.
    if (!game_instance.render || !game_instance.update || !game_instance.init || !game_instance.on_resize) {
        PRINT_ERROR("The game's function pointers must be assigned!");
        return -2;
    }

    // Initialization.
    if (!application_create(&game_instance)) {
        PRINT_INFO("Application failed to create!.");
        return 1;
    }

    // Begin the game loop.
    if(!application_run()) {
        PRINT_INFO("Application did not shutdown gracefully.");
        return 2;
    }

    return 0;
}