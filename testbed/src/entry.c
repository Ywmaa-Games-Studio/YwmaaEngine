#include <entry.h>

#include <core/event.h>
#include "core/ymemory.h"
#include <core/ystring.h>
#include <data_structures/darray.h>
#include <platform/platform.h>

typedef b8 (*PFN_plugin_create)(RENDERER_PLUGIN* out_plugin);
typedef u64 (*PFN_application_state_size)();

b8 load_game_lib(APPLICATION* app) {
    // Dynamically load game library
    if (!platform_dynamic_library_load("testbed_lib_loaded", &app->game_library)) {
        return false;
    }

    if (!platform_dynamic_library_load_function("application_boot", &app->game_library)) {
        return false;
    }
    if (!platform_dynamic_library_load_function("application_init", &app->game_library)) {
        return false;
    }
    if (!platform_dynamic_library_load_function("application_update", &app->game_library)) {
        return false;
    }
    if (!platform_dynamic_library_load_function("application_render", &app->game_library)) {
        return false;
    }
    if (!platform_dynamic_library_load_function("application_on_resize", &app->game_library)) {
        return false;
    }
    if (!platform_dynamic_library_load_function("application_shutdown", &app->game_library)) {
        return false;
    }

    if (!platform_dynamic_library_load_function("application_lib_on_load", &app->game_library)) {
        return false;
    }

    if (!platform_dynamic_library_load_function("application_lib_on_unload", &app->game_library)) {
        return false;
    }

    // assign function pointers
    app->boot = app->game_library.functions[0].pfn;
    app->init = app->game_library.functions[1].pfn;
    app->update = app->game_library.functions[2].pfn;
    app->render = app->game_library.functions[3].pfn;
    app->on_resize = app->game_library.functions[4].pfn;
    app->shutdown = app->game_library.functions[5].pfn;
    app->lib_on_load = app->game_library.functions[6].pfn;
    app->lib_on_unload = app->game_library.functions[7].pfn;

    // Invoke the onload.
    app->lib_on_load(app);

    return true;
}

b8 watched_file_updated(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    if (code == EVENT_CODE_WATCHED_FILE_WRITTEN) {
        APPLICATION* app = (APPLICATION*)listener_inst;
        if (context.data.u32[0] == app->game_library.watch_id) {
            PRINT_INFO("Hot-Reloading game library.");
        }

        // Tell the app it is about to be unloaded.
        app->lib_on_unload(app);

        // Actually unload the app's lib.
        if (!platform_dynamic_library_unload(&app->game_library)) {
            PRINT_ERROR("Failed to unload game library");
            return false;
        }

        // Wait a bit before trying to copy the file.
        platform_sleep(100);

        const char* prefix = platform_dynamic_library_prefix();
        const char* extension = platform_dynamic_library_extension();
        char source_file[260];
        char target_file[260];
        string_format(source_file, "%stestbed_lib%s", prefix, extension);
        string_format(target_file, "%stestbed_lib_loaded%s", prefix, extension);

        E_PLATFORM_ERROR_CODE err_code = PLATFORM_ERROR_FILE_LOCKED;
        while (err_code == PLATFORM_ERROR_FILE_LOCKED) {
            PRINT_TRACE("trying path %s", source_file)
            err_code = platform_copy_file(source_file, target_file, true);
            if (err_code == PLATFORM_ERROR_FILE_LOCKED) {
                platform_sleep(100);
            }
        }

        if (err_code == PLATFORM_ERROR_FILE_NOT_FOUND) {
            yzero_memory(source_file, sizeof(char) * 260);
            string_format(source_file, "../lib/%stestbed_lib%s", prefix, extension);
            PRINT_WARNING("trying different path %s", source_file)
            err_code = PLATFORM_ERROR_FILE_LOCKED;
            while (err_code == PLATFORM_ERROR_FILE_LOCKED) {
                err_code = platform_copy_file(source_file, target_file, true);
                if (err_code == PLATFORM_ERROR_FILE_LOCKED) {
                    platform_sleep(100);
                }
            }
        }

        if (err_code != PLATFORM_ERROR_SUCCESS) {
            PRINT_ERROR("File copy failed!");
            return false;
        }

        if (!load_game_lib(app)) {
            PRINT_ERROR("Game lib reload failed.");
            return false;
        }
    }
    return false;
}

// Define the function to create a game
b8 create_application(APPLICATION* out_application) {
    // Application configuration.
    out_application->app_config.start_pos_x = 100;
    out_application->app_config.start_pos_y = 100;
    out_application->app_config.start_width = 1280;
    out_application->app_config.start_height = 720;
    out_application->app_config.name = "YWMAA Engine Testbed";

    E_PLATFORM_ERROR_CODE err_code = PLATFORM_ERROR_FILE_LOCKED;
    const char* prefix = platform_dynamic_library_prefix();
    const char* extension = platform_dynamic_library_extension();

    while (err_code == PLATFORM_ERROR_FILE_LOCKED) {
        char source_file[260];
        char target_file[260];
        string_format(source_file, "%stestbed_lib%s", prefix, extension);
        string_format(target_file, "%stestbed_lib_loaded%s", prefix, extension);

        PRINT_TRACE("trying path %s", source_file)

        err_code = platform_copy_file(source_file, target_file, true);
        if (err_code == PLATFORM_ERROR_FILE_LOCKED) {
            platform_sleep(100);
        }
    }

    
    if (err_code == PLATFORM_ERROR_FILE_NOT_FOUND) {
        err_code = PLATFORM_ERROR_FILE_LOCKED;
        while (err_code == PLATFORM_ERROR_FILE_LOCKED) {
            char source_file[260];
            char target_file[260];
            string_format(source_file, "../lib/%stestbed_lib%s", prefix, extension);
            string_format(target_file, "%stestbed_lib_loaded%s", prefix, extension);

            PRINT_WARNING("trying different path %s", source_file)

            err_code = platform_copy_file(source_file, target_file, true);
            if (err_code == PLATFORM_ERROR_FILE_LOCKED) {
                platform_sleep(100);
            }
        }
    }

    if (err_code != PLATFORM_ERROR_SUCCESS) {
        PRINT_ERROR("File copy failed! %i", err_code);
        return false;
    }

    if (!load_game_lib(out_application)) {
        PRINT_ERROR("Initial game lib load failed!");
    }

    out_application->engine_state = 0;
    out_application->state = 0;

    switch (out_application->app_config.renderer_backend_api)
    {
        case RENDERER_BACKEND_API_VULKAN:
            if (!platform_dynamic_library_load("vulkan_renderer", &out_application->renderer_library)) {
                PRINT_ERROR("Failed to load vulkan_renderer dynamic_library");
                return false;
            }

            if (!platform_dynamic_library_load_function("plugin_create", &out_application->renderer_library)) {
                PRINT_ERROR("Failed to load vulkan_renderer dynamic_library");
                return false;
            }
            break;
        case RENDERER_BACKEND_API_WEBGPU:
            if (!platform_dynamic_library_load("webgpu_renderer", &out_application->renderer_library)) {
                PRINT_ERROR("Failed to load webgpu_renderer dynamic_library");
                return false;
            }

            if (!platform_dynamic_library_load_function("plugin_create", &out_application->renderer_library)) {
                PRINT_ERROR("Failed to load webgpu_renderer dynamic_library");
                return false;
            }
            break;
        
        default:
            return false;
            break;
    }

    // Create the renderer plugin.
    PFN_plugin_create plugin_create = out_application->renderer_library.functions[0].pfn;
    if (!plugin_create(&out_application->render_plugin)) {
        return false;
    }

    return true;
}

b8 init_application(APPLICATION* app) {
    if (!event_register(EVENT_CODE_WATCHED_FILE_WRITTEN, app, watched_file_updated)) {
        return false;
    }

    const char* prefix = platform_dynamic_library_prefix();
    const char* extension = platform_dynamic_library_extension();
    char path[260];
    yzero_memory(path, sizeof(char) * 260);
    string_format(path, "%s%s%s", prefix, "testbed_lib", extension);

    if (!platform_watch_file(path, &app->game_library.watch_id)) {
        yzero_memory(path, sizeof(char) * 260);
        string_format(path, "../lib/%s%s%s", prefix, "testbed_lib", extension);
        if (!platform_watch_file(path, &app->game_library.watch_id)) {
            PRINT_ERROR("Failed to watch the testbed library!");
            return false;
        }
    }

    return true;
}
