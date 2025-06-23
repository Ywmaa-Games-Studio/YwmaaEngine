#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/ymemory.h"
#include "core/event.h"
#include "input/input.h"
#include "core/clock.h"
#include "core/ystring.h"

#include "memory/linear_allocator.h"

#include "renderer/renderer_frontend.h"

// systems
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

// TODO: temp
#include "math/ymath.h"
#include "math/transform.h"
#include "math/geometry_utils.h"
#include "variants/darray.h"
// TODO: end temp

typedef struct APPLICATION_STATE {
    GAME* game_instance;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;
    LINEAR_ALLOCATOR systems_allocator;

    u64 event_system_memory_requirement;
    void* event_system_state;

    u64 logging_system_memory_requirement;
    void* logging_system_state;

    u64 input_system_memory_requirement;
    void* input_system_state;

    u64 platform_system_memory_requirement;
    void* platform_system_state;

    u64 resource_system_memory_requirement;
    void* resource_system_state;

    u64 shader_system_memory_requirement;
    void* shader_system_state;

    u64 renderer_system_memory_requirement;
    void* renderer_system_state;

    u64 texture_system_memory_requirement;
    void* texture_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

    u64 geometry_system_memory_requirement;
    void* geometry_system_state;

    // TODO: temp
    Mesh meshes[10];
    u32 mesh_count;

    GEOMETRY* test_ui_geometry;
    // TODO: end temp
} APPLICATION_STATE;

static APPLICATION_STATE* app_state;

// Event handlers
b8 application_on_event(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);
b8 application_on_key(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);
b8 application_on_resized(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context);

// TODO: temp
b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT data) {
    const char* names[3] = {
        "cobblestone",
        "paving",
        "paving2"};
    static i8 choice = 2;

    // Save off the old name.
    const char* old_name = names[choice];
    choice++;
    choice %= 3;

    // Acquire the new diffuse texture.
    // Just swap out the material on the first Mesh if it exists.
    GEOMETRY* g = app_state->meshes[0].geometries[0];
    if (g) {
        // Acquire the new material.
        g->material = material_system_acquire(names[choice]);
        if (!g->material) {
            PRINT_WARNING("event_on_debug_event no material found! Using default material.");
            g->material = material_system_get_default();
        }
        // Release the old diffuse material.
        material_system_release(old_name);
    }

    return true;
}
// TODO: end temp

b8 application_create(GAME* game_instance) {
    if (game_instance->application_state) {
        PRINT_ERROR("application_create called more than once.");
        return false;
    }

    // Memory system must be the first thing to be stood up.
    MEMORY_SYSTEM_CONFIG memory_system_config = {0};
    
    memory_system_config.total_alloc_size = MEBIBYTES(1024);
    if (!memory_system_init(memory_system_config)) {
        PRINT_ERROR("Failed to initialize memory system; shutting down.");
        return false;
    }

    // Allocate the game state.
    // TODO:
    //should I make the linear allocator switch to malloc instead of using the engine's allocator?
    // mainly the reasons for this is that we won't track its allocations, it is almost static
    // and it consumes the large pool
    game_instance->state = yallocate(game_instance->state_memory_requirement, MEMORY_TAG_GAME);

    game_instance->application_state = yallocate_aligned(sizeof(APPLICATION_STATE), 8, MEMORY_TAG_APPLICATION);
    app_state = game_instance->application_state;
    app_state->game_instance = game_instance;
    app_state->is_running = false;
    app_state->is_suspended = false;

    // Create a linear allocator for all systems (except memory) to use.
    u64 systems_allocator_total_size = MEBIBYTES(64);
    linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);

    // Initialize other subsystems.

    // Events
    event_system_init(&app_state->event_system_memory_requirement, 0);
    app_state->event_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->event_system_memory_requirement);
    event_system_init(&app_state->event_system_memory_requirement, app_state->event_system_state);

    // Logging
    init_logging(&app_state->logging_system_memory_requirement, 0);
    app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if (!init_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {
        PRINT_ERROR("Failed to initialize logging system; shutting down.");
        return false;
    }

    // Input
    input_system_init(&app_state->input_system_memory_requirement, 0);
    app_state->input_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->input_system_memory_requirement);
    input_system_init(&app_state->input_system_memory_requirement, app_state->input_system_state);

    // Register for engine-level events.
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_RESIZED, 0, application_on_resized);
    // TODO: temp
    event_register(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    // TODO: end temp

    // Platform
    platform_system_startup(&app_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
    app_state->platform_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->platform_system_memory_requirement);
    if (!platform_system_startup(
        &app_state->platform_system_memory_requirement,
        app_state->platform_system_state,
        game_instance->app_config.name,
        game_instance->app_config.start_pos_x,
        game_instance->app_config.start_pos_y,
        game_instance->app_config.start_width,
        game_instance->app_config.start_height)) {
            return false;
    }

    // Resource system.
    RESOURCE_SYSTEM_CONFIG resource_sys_config;
    resource_sys_config.asset_base_path = "assets";
    resource_sys_config.max_loader_count = 32;
    resource_system_init(&app_state->resource_system_memory_requirement, 0, resource_sys_config);
    app_state->resource_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->resource_system_memory_requirement);
    if(!resource_system_init(&app_state->resource_system_memory_requirement, app_state->resource_system_state, resource_sys_config)) {
        PRINT_ERROR("Failed to initialize resource system. Aborting application.");
        return false;
    }
    

    // Allocate the texture & material systems memory before the renderer to not overflow the renderer's memory
    TEXTURE_SYSTEM_CONFIG texture_sys_config;
    texture_sys_config.max_texture_count = 65536;
    texture_system_init(&app_state->texture_system_memory_requirement, 0, texture_sys_config);
    app_state->texture_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->texture_system_memory_requirement);

    MATERIAL_SYSTEM_CONFIG material_sys_config;
    material_sys_config.max_material_count = 4096;
    material_system_init(&app_state->material_system_memory_requirement, 0, material_sys_config);
    app_state->material_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->material_system_memory_requirement);

    GEOMETRY_SYSTEM_CONFIG geometry_sys_config;
    geometry_sys_config.max_geometry_count = 4096;
    geometry_system_init(&app_state->geometry_system_memory_requirement, 0, geometry_sys_config);
    app_state->geometry_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->geometry_system_memory_requirement);
    
    // Shader system
    SHADER_SYSTEM_CONFIG shader_sys_config;
    shader_sys_config.max_shader_count = 1024;
    shader_sys_config.max_uniform_count = 128;
    shader_sys_config.max_global_textures = 31;
    shader_sys_config.max_instance_textures = 31;
    shader_system_init(&app_state->shader_system_memory_requirement, 0, shader_sys_config);
    app_state->shader_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->shader_system_memory_requirement);
    if(!shader_system_init(&app_state->shader_system_memory_requirement, app_state->shader_system_state, shader_sys_config)) {
        PRINT_ERROR("Failed to initialize shader system. Aborting application.");
        return false;
    }

    // Renderer system
    app_state->renderer_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if (!renderer_system_init(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, game_instance->app_config.name, game_instance->app_config.renderer_backend_api)) { //RENDERER_BACKEND_API_WEBGPU|RENDERER_BACKEND_API_VULKAN
        PRINT_ERROR("Failed to initialize renderer. Aborting application.");
        return false;
    }

    // Texture system.
    if (!texture_system_init(&app_state->texture_system_memory_requirement, app_state->texture_system_state, texture_sys_config)) {
        PRINT_ERROR("Failed to initialize texture system. Application cannot continue.");
        return false;
    }
    
    // Material system.
    if (!material_system_init(&app_state->material_system_memory_requirement, app_state->material_system_state, material_sys_config)) {
        PRINT_ERROR("Failed to initialize material system. Application cannot continue.");
        return false;
    }
   
    // Geometry system.
    if (!geometry_system_init(&app_state->geometry_system_memory_requirement, app_state->geometry_system_state, geometry_sys_config)) {
        PRINT_ERROR("Failed to initialize geometry system. Application cannot continue.");
        return false;
    }
    
    // TODO: temp 
    app_state->mesh_count = 0;

    // Load up a cube configuration, and load geometry from it.
    Mesh* cube_mesh = &app_state->meshes[app_state->mesh_count];
    cube_mesh->geometry_count = 1;
    cube_mesh->geometries = yallocate_aligned(sizeof(Mesh*) * cube_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
    GEOMETRY_CONFIG g_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");
    cube_mesh->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh->transform = transform_create();
    app_state->mesh_count++;
    // Clean up the allocations for the geometry config.
    geometry_system_config_dispose(&g_config);


    // A second cube
    Mesh* cube_mesh_2 = &app_state->meshes[app_state->mesh_count];
    cube_mesh_2->geometry_count = 1;
    cube_mesh_2->geometries = yallocate_aligned(sizeof(Mesh*) * cube_mesh_2->geometry_count, 8, MEMORY_TAG_ARRAY);
    g_config = geometry_system_generate_cube_config(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube_2", "test_material");
    cube_mesh_2->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh_2->transform = transform_from_position((Vector3){10.0f, 0.0f, 1.0f});
    // Set the first cube as the parent to the second.
    transform_set_parent(&cube_mesh_2->transform, &cube_mesh->transform);
    app_state->mesh_count++;
    // Clean up the allocations for the geometry config.
    geometry_system_config_dispose(&g_config);

    // A third cube!
    Mesh* cube_mesh_3 = &app_state->meshes[app_state->mesh_count];
    cube_mesh_3->geometry_count = 1;
    cube_mesh_3->geometries = yallocate_aligned(sizeof(Mesh*) * cube_mesh_3->geometry_count, 8, MEMORY_TAG_ARRAY);
    g_config = geometry_system_generate_cube_config(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube_2", "test_material");
    cube_mesh_3->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh_3->transform = transform_from_position((Vector3){5.0f, 0.0f, 1.0f});
    // Set the second cube as the parent to the third.
    transform_set_parent(&cube_mesh_3->transform, &cube_mesh_2->transform);
    app_state->mesh_count++;
    // Clean up the allocations for the geometry config.
    geometry_system_config_dispose(&g_config);

    // test loading mesh from file
    Mesh* car_mesh = &app_state->meshes[app_state->mesh_count];
    RESOURCE car_mesh_resource = {0};
    if (!resource_system_load("falcon", RESOURCE_TYPE_MESH, &car_mesh_resource)) {
        PRINT_ERROR("Failed to load car mesh resource.");
    } else {
        GEOMETRY_CONFIG* configs = (GEOMETRY_CONFIG*)car_mesh_resource.data;
        car_mesh->geometry_count = car_mesh_resource.data_size;
        car_mesh->geometries = yallocate_aligned(sizeof(Mesh*) * car_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
        for (u32 i = 0; i < car_mesh->geometry_count; ++i) {
            // Acquire the geometry from the config.
            car_mesh->geometries[i] = geometry_system_acquire_from_config(configs[i], true);
        }
        car_mesh->transform = transform_from_position((Vector3){15.0f, 0.0f, 1.0f});
        resource_system_unload(&car_mesh_resource);
        app_state->mesh_count++;
    }

    // A bigger test mesh
    Mesh* sponza_mesh = &app_state->meshes[app_state->mesh_count];
    RESOURCE sponza_mesh_resource = {0};
    if (!resource_system_load("sponza", RESOURCE_TYPE_MESH, &sponza_mesh_resource)) {
        PRINT_ERROR("Failed to load sponza mesh!");
    } else{
        GEOMETRY_CONFIG* sponza_configs = (GEOMETRY_CONFIG*)sponza_mesh_resource.data;
        sponza_mesh->geometry_count = sponza_mesh_resource.data_size;
        if (sponza_mesh->geometry_count == 0) {
            PRINT_ERROR("Sponza mesh has no geometries!");
            return false;
        }
        sponza_mesh->geometries = yallocate_aligned(sizeof(Mesh*) * sponza_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
        for (u32 i = 0; i < sponza_mesh->geometry_count; ++i) {
            sponza_mesh->geometries[i] = geometry_system_acquire_from_config(sponza_configs[i], true);
        }
        sponza_mesh->transform = transform_from_position_rotation_scale((Vector3){15.0f, 0.0f, 1.0f}, Quaternion_identity(), (Vector3){0.05f, 0.05f, 0.05f});
        resource_system_unload(&sponza_mesh_resource);
        app_state->mesh_count++;
    }

    Mesh* helmet_mesh = &app_state->meshes[app_state->mesh_count];
    RESOURCE helmet_mesh_resource = {0};
    if (!resource_system_load("FlightHelmet", RESOURCE_TYPE_MESH, &helmet_mesh_resource)) {
        PRINT_ERROR("Failed to load helmet mesh!");
    } else {
        GEOMETRY_CONFIG* helmet_configs = (GEOMETRY_CONFIG*)helmet_mesh_resource.data;
        helmet_mesh->geometry_count = helmet_mesh_resource.data_size;
        if (helmet_mesh->geometry_count == 0) {
            PRINT_ERROR("helmet mesh has no geometries!");
            return false;
        }
        helmet_mesh->geometries = yallocate_aligned(sizeof(Mesh*) * helmet_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
        for (u32 i = 0; i < helmet_mesh->geometry_count; ++i) {
            helmet_mesh->geometries[i] = geometry_system_acquire_from_config(helmet_configs[i], true);
        }
        helmet_mesh->transform = transform_from_position_rotation_scale((Vector3){0.0f, 5.0f, 0.0f}, Quaternion_identity(), (Vector3){10.0f, 10.0f, 10.0f});
        resource_system_unload(&helmet_mesh_resource);
        app_state->mesh_count++;
    }
    
    Mesh* duck_mesh = &app_state->meshes[app_state->mesh_count];
    RESOURCE duck_mesh_resource = {0};
    if (!resource_system_load("Duck", RESOURCE_TYPE_MESH, &duck_mesh_resource)) {
        PRINT_ERROR("Failed to load duck mesh!");
    } else {
        GEOMETRY_CONFIG* duck_configs = (GEOMETRY_CONFIG*)duck_mesh_resource.data;
        duck_mesh->geometry_count = duck_mesh_resource.data_size;
        if (duck_mesh->geometry_count == 0) {
            PRINT_ERROR("duck mesh has no geometries!");
            return false;
        }
        duck_mesh->geometries = yallocate_aligned(sizeof(Mesh*) * duck_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
        for (u32 i = 0; i < duck_mesh->geometry_count; ++i) {
            duck_mesh->geometries[i] = geometry_system_acquire_from_config(duck_configs[i], true);
        }
        duck_mesh->transform = transform_from_position_rotation_scale((Vector3){0.0f, 15.0f, 0.0f}, Quaternion_identity(), (Vector3){0.025f, 0.025f, 0.025f});
        resource_system_unload(&duck_mesh_resource);
        app_state->mesh_count++;
    }
    // TODO: end temp 

    // Load up some test UI geometry.
    GEOMETRY_CONFIG ui_config;
    ui_config.vertex_size = sizeof(Vertex2D);
    ui_config.vertex_count = 4;
    ui_config.index_size = sizeof(u32);
    ui_config.index_count = 6;
    string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
    string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

    const f32 w = 128.0f;
    const f32 h = 128.0f;
    Vertex2D uiverts [4];
    uiverts[0].position.x = 0.0f;  // 0    3
    uiverts[0].position.y = 0.0f;  //
    uiverts[0].texcoord.x = 0.0f;  //
    uiverts[0].texcoord.y = 0.0f;  // 2    1

    uiverts[1].position.y = h;
    uiverts[1].position.x = w;
    uiverts[1].texcoord.x = 1.0f;
    uiverts[1].texcoord.y = 1.0f;

    uiverts[2].position.x = 0.0f;
    uiverts[2].position.y = h;
    uiverts[2].texcoord.x = 0.0f;
    uiverts[2].texcoord.y = 1.0f;

    uiverts[3].position.x = w;
    uiverts[3].position.y = 0.0;
    uiverts[3].texcoord.x = 1.0f;
    uiverts[3].texcoord.y = 0.0f;
    ui_config.vertices = uiverts;
    
    // Indices - counter-clockwise
    u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
    ui_config.indices = uiindices;

    // Get UI geometry from config.
    app_state->test_ui_geometry = geometry_system_acquire_from_config(ui_config, true);




    // Initialize the game.
    if (!app_state->game_instance->init(app_state->game_instance)) {
        PRINT_ERROR("Game failed to initialize.");
        return false;
    }

    // Call resize once to ensure the proper size has been set.
    app_state->game_instance->on_resize(app_state->game_instance, app_state->width, app_state->height);
    
    return true;
}
b8 application_run(void) {
    app_state->is_running = true;
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed;
    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;

    // This is just a stupid way to bypass the warning of them not being used
    running_time = frame_count;
    frame_count = running_time;

    char* mem_usage = get_memory_usage_str();
    PRINT_INFO(mem_usage);

    while (app_state->is_running) { // Game Loop
        if(!platform_pump_messages()) {
            app_state->is_running = false;
        }

        if(!app_state->is_suspended) {
            // Update clock and get delta time.
            clock_update(&app_state->clock);
            f64 current_time = app_state->clock.elapsed;
            f64 delta = (current_time - app_state->last_time);
            f64 frame_start_time = platform_get_absolute_time();
    
            if (!app_state->game_instance->update(app_state->game_instance, (f32)delta)) {
                PRINT_ERROR("Game update failed, shutting down.");
                app_state->is_running = false;
                break;
            }

            // Call the game's render routine.
            if (!app_state->game_instance->render(app_state->game_instance, (f32)delta)) {
                PRINT_ERROR("Game render failed, shutting down.");
                app_state->is_running = false;
                break;
            }

            // TODO: refactor packet creation
            RENDER_PACKET packet = {0};
            packet.delta_time = delta;
            packet.geometry_count = 0;

            // NOTE: Yes, I know this allocates/frees every framr. No, it doesn't matter because
            // this is temporary.
            packet.geometries = darray_create(GEOMETRY_RENDER_DATA);


            if (app_state->mesh_count > 0) {
                // Perform a small rotation on the first mesh.
                Quaternion rotation = Quaternion_from_axis_angle((Vector3){0, 1, 0}, 0.5f * delta, false);
                transform_rotate(&app_state->meshes[0].transform, rotation);

                // Perform a similar rotation on the second mesh, if it exists.
                if (app_state->mesh_count > 1) {
                    transform_rotate(&app_state->meshes[1].transform, rotation);
                }

                // Perform a similar rotation on the third mesh, if it exists.
                if (app_state->mesh_count > 2) {
                    transform_rotate(&app_state->meshes[2].transform, rotation);
                }

                // Iterate all meshes and add them to the packet's geometries collection
                for (u32 i = 0; i < app_state->mesh_count; ++i) {
                    Mesh* m = &app_state->meshes[i];
                    for (u32 j = 0; j < m->geometry_count; ++j) {
                        GEOMETRY_RENDER_DATA data;
                        data.geometry = m->geometries[j];
                        data.model = transform_get_world(&m->transform);
                        darray_push(packet.geometries, data);
                        packet.geometry_count++;
                    }
                }
            }

            GEOMETRY_RENDER_DATA test_ui_render;
            test_ui_render.geometry = app_state->test_ui_geometry;
            test_ui_render.model = Matrice4_translation((Vector3){0, 0, 0});
            packet.ui_geometry_count = 1;
            packet.ui_geometries = &test_ui_render;

            renderer_draw_frame(&packet);

            // TODO: temp
            // Cleanup the packet.
            if (packet.geometries) {
                darray_destroy(packet.geometries);
                packet.geometries = 0;
            }
            // TODO: end temp

            // Figure out how long the frame took and, if below
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
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
            app_state->last_time = current_time;
        }
    }

    app_state->is_running = false;

    // Shutdown event system.
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    // TODO: temp
    event_unregister(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    // TODO: end temp

    input_system_shutdown(app_state->input_system_state);

    geometry_system_shutdown(app_state->geometry_system_state);

    material_system_shutdown(app_state->material_system_state);

    texture_system_shutdown(app_state->texture_system_state);

    shader_system_shutdown(app_state->shader_system_state);

    renderer_system_shutdown(app_state->renderer_system_state);

    resource_system_shutdown(app_state->resource_system_state);

    platform_system_shutdown(app_state->platform_system_state);

    event_system_shutdown(app_state->event_system_state);

    memory_system_shutdown();

    return true;
}

void application_get_framebuffer_size(u32* width, u32* height) {
    *width = app_state->width;
    *height = app_state->height;
}

b8 application_on_event(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            PRINT_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state->is_running = false;
            return true;
        }
    }

    return false;
}

b8 application_on_key(u16 code, void* sender, void* listener_instance, EVENT_CONTEXT context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            EVENT_CONTEXT data = {0};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            PRINT_DEBUG("Explicit - A key pressed!");
        } else {
            PRINT_DEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            PRINT_DEBUG("Explicit - B key released!");
        } else {
            PRINT_DEBUG("'%c' key released in window.", key_code);
        }
    }
    return false;
}

b8 application_on_resized(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Check if different. If so, trigger a resize event.
        if (width != app_state->width || height != app_state->height) {
            app_state->width = width;
            app_state->height = height;

            PRINT_DEBUG("Window resize: %i, %i", width, height);

            // Handle minimization
            if (width == 0 || height == 0) {
                PRINT_INFO("Window minimized, suspending application.");
                app_state->is_suspended = true;
                return true;
            } else {
                if (app_state->is_suspended) {
                    PRINT_INFO("Window restored, resuming application.");
                    app_state->is_suspended = false;
                }
                app_state->game_instance->on_resize(app_state->game_instance, width, height);
                renderer_on_resized(width, height);
            }
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}


