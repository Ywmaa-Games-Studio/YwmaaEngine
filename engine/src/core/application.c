#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/ymemory.h"
typedef struct APPLICATION_STATE {
    GAME* game_instance;
    b8 is_running;
    b8 is_suspended;
    PLATFORM_STATE platform;
    i16 width;
    i16 height;
    f64 last_time;
} APPLICATION_STATE;

static b8 initialized = FALSE;
static APPLICATION_STATE app_state;

b8 application_create(GAME* game_instance) {
    if (initialized) {
        PRINT_ERROR("application_create called more than once.");
        return FALSE;
    }

    app_state.game_instance = game_instance;

    // Initialize subsystems.
    init_logging();

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;

    if (!platform_startup(
            &app_state.platform,
            game_instance->app_config.name,
            game_instance->app_config.start_pos_x,
            game_instance->app_config.start_pos_y,
            game_instance->app_config.start_width,
            game_instance->app_config.start_height)) {
        return FALSE;
    }

    // Initialize the game.
    if (!app_state.game_instance->init(app_state.game_instance)) {
        PRINT_ERROR("Game failed to initialize.");
        return FALSE;
    }

    app_state.game_instance->on_resize(app_state.game_instance, app_state.width, app_state.height);

    initialized = TRUE;

    return TRUE;
}

b8 application_run() {
    while (app_state.is_running) { // Game Loop
        if(!platform_pump_messages(&app_state.platform)) {
            app_state.is_running = FALSE;
        }

        if(!app_state.is_suspended) {
            if (!app_state.game_instance->update(app_state.game_instance, (f32)0)) {
                PRINT_ERROR("Game update failed, shutting down.");
                app_state.is_running = FALSE;
                break;
            }

            // Call the game's render routine.
            if (!app_state.game_instance->render(app_state.game_instance, (f32)0)) {
                PRINT_ERROR("Game render failed, shutting down.");
                app_state.is_running = FALSE;
                break;
            }
        }
    }

    app_state.is_running = FALSE;

    platform_shutdown(&app_state.platform);

    return TRUE;
}