#include "engine.h"
#include "application_types.h"

#include "version.h"

#include "platform/platform.h"
#include "core/logger.h"
#include "core/ymemory.h"
#include "core/event.h"
#include "input/input.h"
#include "core/clock.h"
#include "core/ystring.h"
#include "core/uuid.h"
#include "core/metrics.h"

#include "data_structures/darray.h"

#include "renderer/renderer_frontend.h"

// systems
#include "core/systems_manager.h"

typedef struct ENGINE_STATE_T {
    APPLICATION* game_instance;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;
    SYSTEMS_MANAGER_STATE sys_manager_state;
} ENGINE_STATE_T;

static ENGINE_STATE_T* engine_state;

// Event handlers
b8 engine_on_event(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);
b8 engine_on_resized(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);

b8 engine_create(APPLICATION* game_instance) {
    if (game_instance->engine_state) {
        PRINT_ERROR("engine_create called more than once.");
        return false;
    }

    // Memory system must be the first thing to be stood up.
    MEMORY_SYSTEM_CONFIG memory_system_config = {0};
    memory_system_config.total_alloc_size = MEBIBYTES(1024);
    if (!memory_system_init(memory_system_config)) {
        PRINT_ERROR("Failed to initialize memory system; shutting down.");
        return false;
    }

    // Seed the uuid generator.
    // TODO: A better seed here.
    uuid_seed(1729);

    // Metrics
    metrics_init();

    // Allocate the game state.
    // TODO:
    //should I make the linear allocator switch to malloc instead of using the engine's allocator?
    // mainly the reasons for this is that we won't track its allocations, it is almost static
    // and it consumes the large pool
    game_instance->state = yallocate(game_instance->state_memory_requirement, MEMORY_TAG_GAME);

    game_instance->engine_state = yallocate_aligned(sizeof(ENGINE_STATE_T), 8, MEMORY_TAG_ENGINE);
    engine_state = game_instance->engine_state;
    engine_state->game_instance = game_instance;
    engine_state->is_running = false;
    engine_state->is_suspended = false;

    if (!systems_manager_init(&engine_state->sys_manager_state, &game_instance->app_config)) {
        PRINT_ERROR("Systems manager failed to initialize. Aborting process.");
        return false;
    }

    // Perform the game's boot sequence.
    if (!game_instance->boot(game_instance)) {
        PRINT_ERROR("Game boot sequence failed; aborting application.");
        return false;
    }

    if (!systems_manager_post_boot_init(&engine_state->sys_manager_state, &game_instance->app_config)) {
        PRINT_ERROR("Post-boot system manager initialization failed!");
        return false;
    }

    // Report engine version
    PRINT_INFO("Ywmaa Engine v. %s", YVERSION);

    // Initialize the game.
    if (!engine_state->game_instance->init(engine_state->game_instance)) {
        PRINT_ERROR("Game failed to initialize.");
        return false;
    }

    // Call resize once to ensure the proper size has been set.
    renderer_on_resized(engine_state->width, engine_state->height);
    engine_state->game_instance->on_resize(engine_state->game_instance, engine_state->width, engine_state->height);
    
    return true;
}

b8 engine_run(void) {
    engine_state->is_running = true;
    clock_start(&engine_state->clock);
    clock_update(&engine_state->clock);
    engine_state->last_time = engine_state->clock.elapsed;
    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;
    f64 frame_elapsed_time = 0;

    // This is just a stupid way to bypass the warning of them not being used
    running_time = frame_count;
    frame_count = running_time;

    char* mem_usage = get_memory_usage_str();
    PRINT_INFO(mem_usage);

    while (engine_state->is_running) { // Game Loop
        if(!platform_pump_messages()) {
            engine_state->is_running = false;
        }

        if(!engine_state->is_suspended) {
            // Update clock and get delta time.
            clock_update(&engine_state->clock);
            f64 current_time = engine_state->clock.elapsed;
            f64 delta = (current_time - engine_state->last_time);
            f64 frame_start_time = platform_get_absolute_time();

            // Update systems.
            systems_manager_update(&engine_state->sys_manager_state, delta);

            // update metrics
            metrics_update(frame_elapsed_time);
    
            if (!engine_state->game_instance->update(engine_state->game_instance, (f32)delta)) {
                PRINT_ERROR("Game update failed, shutting down.");
                engine_state->is_running = false;
                break;
            }

            // TODO: refactor packet creation
            RENDER_PACKET packet = {0};
            packet.delta_time = delta;

            // Call the game's render routine.
            if (!engine_state->game_instance->render(engine_state->game_instance, &packet, (f32)delta)) {
                PRINT_ERROR("Game render failed, shutting down.");
                engine_state->is_running = false;
                break;
            }

            renderer_draw_frame(&packet);

            // Cleanup the packet.
            for (u32 i = 0; i < packet.view_count; ++i) {
                packet.views[i].view->on_destroy_packet(packet.views[i].view, &packet.views[i]);
            }

            // Figure out how long the frame took and, if below
            f64 frame_end_time = platform_get_absolute_time();
            frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

            if (remaining_seconds > 0) {
                u64 remaining_ms = (remaining_seconds * 1000);

                // If there is time left, give it back to the OS.
                b8 limit_frames = false;
                if (remaining_ms > 0 && limit_frames) {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;
            }

            // NOTE: Input update/state copying should always be handled
            // after any input should be recorded; I.E. before this line.
            // As a safety, input is the last thing to be updated before
            // this frame ends.
            input_update(delta);

            // Update last time
            engine_state->last_time = current_time;
        }
    }

    engine_state->is_running = false;

    // Shut down the game.
    engine_state->game_instance->shutdown(engine_state->game_instance);

    // Unregister from events.
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, engine_on_event);

    // Shut down all systems.
    systems_manager_shutdown(&engine_state->sys_manager_state);

    return true;
}

void engine_get_framebuffer_size(u32* width, u32* height) {
    *width = engine_state->width;
    *height = engine_state->height;
}

void engine_on_event_system_initialized(void) {
    // Register for engine-level events.
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, engine_on_event);
    event_register(EVENT_CODE_RESIZED, 0, engine_on_resized);
}

b8 engine_on_event(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            PRINT_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            engine_state->is_running = false;
            return true;
        }
    }

    return false;
}

b8 engine_on_resized(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Check if different. If so, trigger a resize event.
        if (width != engine_state->width || height != engine_state->height) {
            engine_state->width = width;
            engine_state->height = height;

            PRINT_DEBUG("Window resize: %i, %i", width, height);

            // Handle minimization
            if (width == 0 || height == 0) {
                PRINT_INFO("Window minimized, suspending application.");
                engine_state->is_suspended = true;
                return true;
            } else {
                if (engine_state->is_suspended) {
                    PRINT_INFO("Window restored, resuming application.");
                    engine_state->is_suspended = false;
                }
                engine_state->game_instance->on_resize(engine_state->game_instance, width, height);
                renderer_on_resized(width, height);
            }
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}
