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
#include "core/frame_data.h"

#include "data_structures/darray.h"

#include "renderer/renderer_frontend.h"
#include "memory/linear_allocator.h"

// systems
#include "core/systems_manager.h"

typedef struct ENGINE_STATE_T {
    APPLICATION* game_instance;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    native_clock clock;
    f64 last_time;
    SYSTEMS_MANAGER_STATE sys_manager_state;
    // An allocator used for per-frame allocations, that is reset every frame.
    LINEAR_ALLOCATOR frame_allocator;
    FRAME_DATA p_frame_data;
} ENGINE_STATE_T;

static ENGINE_STATE_T* engine_state;

// Event handlers
static b8 engine_on_event(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);
static b8 engine_on_resized(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);

b8 engine_create(APPLICATION* game_instance) {
    if (game_instance->engine_state) {
        PRINT_ERROR("engine_create called more than once.");
        return false;
    }

    // Memory system must be the first thing to be stood up.
    MEMORY_SYSTEM_CONFIG memory_system_config = {0};
    memory_system_config.total_alloc_size = GIBIBYTES(1);
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
    //game_instance->state = yallocate(game_instance->state_memory_requirement, MEMORY_TAG_GAME);

    game_instance->engine_state = yallocate_aligned(sizeof(ENGINE_STATE_T), 8, MEMORY_TAG_ENGINE);
    engine_state = game_instance->engine_state;
    engine_state->game_instance = game_instance;
    engine_state->is_running = false;
    engine_state->is_suspended = false;

    game_instance->app_config.renderer_plugin = game_instance->render_plugin;

    if (!systems_manager_init(&engine_state->sys_manager_state, &game_instance->app_config)) {
        PRINT_ERROR("Systems manager failed to initialize. Aborting process.");
        return false;
    }

    // Perform the game's boot sequence.
    game_instance->stage = APPLICATION_STAGE_BOOTING;
    if (!game_instance->boot(game_instance)) {
        PRINT_ERROR("Game boot sequence failed; aborting application.");
        return false;
    }
    // Setup the frame allocator.
    linear_allocator_create(game_instance->app_config.frame_allocator_size, 0, &engine_state->frame_allocator);
    engine_state->p_frame_data.frame_allocator = &engine_state->frame_allocator;
    if (game_instance->app_config.app_frame_data_size > 0) {
        engine_state->p_frame_data.application_frame_data = yallocate_aligned(game_instance->app_config.app_frame_data_size, 8, MEMORY_TAG_GAME);
    } else {
        engine_state->p_frame_data.application_frame_data = 0;
    }
    game_instance->stage = APPLICATION_STAGE_BOOT_COMPLETE;
#ifndef YPLATFORM_WEB
    return engine_post_boot();
#else
    return true;
#endif
}

b8 engine_post_boot(void) {
    if (!systems_manager_post_boot_init(&engine_state->sys_manager_state, &engine_state->game_instance->app_config)) {
        PRINT_ERROR("Post-boot system manager initialization failed!");
        return false;
    }

    // Report engine version
    PRINT_INFO("Ywmaa Engine v. %s", YVERSION);

    // Initialize the game.
    engine_state->game_instance->stage = APPLICATION_STAGE_INITIALIZING;
    if (!engine_state->game_instance->init(engine_state->game_instance)) {
        PRINT_ERROR("Game failed to initialize.");
        return false;
    }
    engine_state->game_instance->stage = APPLICATION_STAGE_INITIALIZED;

    // Call resize once to ensure the proper size has been set.
    renderer_on_resized(engine_state->width, engine_state->height);
    engine_state->game_instance->on_resize(engine_state->game_instance, engine_state->width, engine_state->height);

    return true;
}

#ifdef YPLATFORM_WEB
b8 web_pause = false;
#endif
f64 running_time = 0;
//u8 frame_count = 0;
f64 target_frame_seconds = 1.0f / 60;
f64 frame_elapsed_time = 0;

b8 render_loop(void) {
    if(!platform_pump_messages()) {
        engine_state->is_running = false;
    }

    if(!engine_state->is_suspended) {
        // Update clock and get delta time.
        clock_update(&engine_state->clock);
        f64 current_time = engine_state->clock.elapsed;
        f64 delta = (current_time - engine_state->last_time);
    #ifndef YPLATFORM_WEB
        f64 frame_start_time = platform_get_absolute_time();
    #endif
        engine_state->p_frame_data.total_time = current_time;
        engine_state->p_frame_data.delta_time = (f32)delta;

        // Reset the frame allocator
        linear_allocator_free_all(&engine_state->frame_allocator);

        // Update systems.
        systems_manager_update(&engine_state->sys_manager_state, &engine_state->p_frame_data);

        // update metrics
        metrics_update(frame_elapsed_time);

        if (!engine_state->game_instance->update(engine_state->game_instance, &engine_state->p_frame_data)) {
            PRINT_ERROR("Game update failed, shutting down.");
            engine_state->is_running = false;
            return false;
        }

        // TODO: refactor packet creation
        RENDER_PACKET packet = {0};

        // Call the game's render routine.
        if (!engine_state->game_instance->render(engine_state->game_instance, &packet, &engine_state->p_frame_data)) {
            PRINT_ERROR("Game render failed, shutting down.");
            engine_state->is_running = false;
            return false;
        }

        renderer_draw_frame(&packet, &engine_state->p_frame_data);

        // Cleanup the packet.
        for (u32 i = 0; i < packet.view_count; ++i) {
            packet.views[i].view->on_packet_destroy(packet.views[i].view, &packet.views[i]);
        }

    #ifndef YPLATFORM_WEB
        // Figure out how long the frame took and, if below
        f64 frame_end_time = platform_get_absolute_time();
        frame_elapsed_time = frame_end_time - frame_start_time;
    #endif
        running_time += frame_elapsed_time;
        f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

        if (remaining_seconds > 0) {
            u64 remaining_ms = (remaining_seconds * 1000);

            // If there is time left, give it back to the OS.
            b8 limit_frames = false;
            if (remaining_ms > 0 && limit_frames) {
                #ifndef YPLATFORM_WEB
                platform_sleep(remaining_ms - 1);
                #else

                #endif
            }

            //frame_count++;
        }

        // NOTE: Input update/state copying should always be handled
        // after any input should be recorded; I.E. before this line.
        // As a safety, input is the last thing to be updated before
        // this frame ends.
        input_update(&engine_state->p_frame_data);

        // Update last time
        engine_state->last_time = current_time;
    }
    return true;
}

void post_render_shutdown(void) {
    engine_state->is_running = false;
    engine_state->game_instance->stage = APPLICATION_STAGE_SHUTTING_DOWN;

    // Shut down the game.
    engine_state->game_instance->shutdown(engine_state->game_instance);

    // Unregister from events.
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, engine_on_event);

    engine_state->game_instance->stage = APPLICATION_STAGE_UNINITIALIZED;

    // Shut down all systems.
    systems_manager_shutdown(&engine_state->sys_manager_state);

}
#ifdef YPLATFORM_WEB
#include <emscripten.h>
#include <emscripten/html5.h>

static double last_time = 0.0;
// Callback type takes one argument of type 'void*' and returns nothing
//typedef void (*em_arg_callback_func)(void*);
EMSCRIPTEN_KEEPALIVE
EM_BOOL em_loop(double time, void *userData) {
    double current_time = time * 0.001; // convert ms to seconds
    frame_elapsed_time = current_time - last_time;
    if (last_time == 0.0) {
        last_time = current_time;
        emscripten_request_animation_frame(em_loop, NULL);
        return EM_TRUE;
    }

    double dt = current_time - last_time;
    b8 limit_frames = false;
    if (dt < target_frame_seconds && limit_frames == true) {
        // Not enough time passed, skip this frame, request next one
        emscripten_request_animation_frame(em_loop, NULL);
        return EM_TRUE;
    }

    last_time = current_time;

    int result = render_loop();

    if (result == 0) {
        post_render_shutdown();
        return EM_FALSE;
    }

    emscripten_request_animation_frame(em_loop, NULL);
    return EM_TRUE;
}

APPLICATION* get_application_instance(void) {
    return engine_state->game_instance;
}
#endif

b8 engine_run(APPLICATION* game_instance) {
    game_instance->stage = APPLICATION_STAGE_RUNNING;
    engine_state->is_running = true;
    clock_start(&engine_state->clock);
    clock_update(&engine_state->clock);
    engine_state->last_time = engine_state->clock.elapsed;

    char* mem_usage = get_memory_usage_str();
    PRINT_INFO(mem_usage);

#ifndef YPLATFORM_WEB
    while (engine_state->is_running) { // Game Loop
        if (!render_loop()){
            break;
        }
    }
    post_render_shutdown();

#else
    emscripten_request_animation_frame(em_loop, NULL);
    //emscripten_request_animation_frame_loop(em_loop, game_instance);
#endif

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

const struct FRAME_DATA* engine_frame_data_get(struct APPLICATION* game_instance) {
    return &((ENGINE_STATE_T*)game_instance->engine_state)->p_frame_data;
}

static b8 engine_on_event(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            PRINT_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            engine_state->is_running = false;
            return true;
        }
    }

    return false;
}

static b8 engine_on_resized(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
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
