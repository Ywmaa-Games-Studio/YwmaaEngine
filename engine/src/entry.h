#pragma once

#include "core/engine.h"
#include "core/logger.h"
#include "renderer/renderer_types.inl"
#include "application_types.h"

// Externally-defined function to create a application.
extern b8 create_application(APPLICATION* out_app);
void run_benchmarks();

/**
 * The main entry point of the application in C.
 * This is actually called from the main function in main.zig
 */
int engine_main(E_RENDERER_BACKEND_API renderer_backend_api) {
    // I use this to quickly test the memory system.
/*     run_benchmarks();
    return 0; */
    
    // Request the application instance from the application.
    APPLICATION application_instance;
    application_instance.app_config.renderer_backend_api = renderer_backend_api;
    if (!create_application(&application_instance)) {
        PRINT_ERROR("Could not create application!");
        return -1;
    }

    // Ensure the function pointers exist.
    if (!application_instance.render || !application_instance.update || !application_instance.init || !application_instance.on_resize) {
        PRINT_ERROR("The application's function pointers must be assigned!");
        return -2;
    }

    // Initialization.
    if (!engine_create(&application_instance)) {
        PRINT_INFO("Application failed to create!.");
        return 1;
    }

    // Begin the engine loop.
    if(!engine_run()) {
        PRINT_INFO("Application did not shutdown gracefully.");
        return 2;
    }

    return 0;
}