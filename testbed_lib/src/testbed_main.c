#include "testbed_main.h"
#include "game_state.h"

#include <core/logger.h>
#include <core/ystring.h>
#include <input/input.h>
#include <core/event.h>
#include <core/metrics.h>
#include <core/clock.h>
#include <core/console.h>

#include <data_structures/darray.h>

#include <math/ymath.h>
#include <renderer/renderer_types.inl>
#include <renderer/renderer_frontend.h>

// TODO: temp
#include <core/identifier.h>
#include <math/transform.h>
#include <resources/skybox.h>
#include <resources/ui_text.h>
#include <resources/mesh.h>
#include <systems/geometry_system.h>
#include <systems/material_system.h>
#include <systems/render_view_system.h>
#include "debug_console.h"
#include "game_commands.h"
#include "game_keybinds.h"
// TODO: end temp

b8 configure_render_views(APPLICATION_CONFIG* config);
void application_register_events(struct APPLICATION* application_instance);
void application_unregister_events(struct APPLICATION* application_instance);

b8 application_on_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    APPLICATION* application_instance = (APPLICATION*)listener_inst;
    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    switch (code) {
        case EVENT_CODE_OBJECT_HOVER_ID_CHANGED: {
            state->hovered_object_id = context.data.u32[0];
            return true;
        }
    }

    return false;
}

b8 application_on_debug_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT data) {
    APPLICATION* application_instance = (APPLICATION*)listener_inst;
    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    if (code == EVENT_CODE_DEBUG0) {
        const char* names[3] = {
            "cobblestone",
            "paving",
            "paving2"};
        static i8 choice = 2;

        // Save off the old names.
        const char* old_name = names[choice];

        choice++;
        choice %= 3;

        // Just swap out the material on the first mesh if it exists.
        GEOMETRY* g = state->meshes[0].geometries[0];
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
    } else if (code == EVENT_CODE_DEBUG1) {
        if (!state->models_loaded) {
            PRINT_DEBUG("Loading models...");
            state->models_loaded = true;
            if (!mesh_load_from_resource("falcon", state->car_mesh)) {
                PRINT_ERROR("Failed to load falcon mesh!");
            }
            if (!mesh_load_from_resource("sponza", state->sponza_mesh)) {
                PRINT_ERROR("Failed to load falcon mesh!");
            }
            if (!mesh_load_from_resource("FlightHelmet", state->helmet_mesh)) {
                PRINT_ERROR("Failed to load helmet mesh!");
            }
            if (!mesh_load_from_resource("Duck", state->duck_mesh)) {
                PRINT_ERROR("Failed to load duck mesh!");
            }
        }
        return true;
    }

    return false;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    // if (code == EVENT_CODE_KEY_PRESSED) {
    //     u16 key_code = context.data.u16[0];
    //     if (key_code == KEY_A) {
    //         // Example on checking for a key
    //         KDEBUG("Explicit - A key pressed!");
    //     } else {
    //         // KTRACE("'%s' key pressed in window.", input_keycode_str(key_code));
    //     }
    // } else if (code == EVENT_CODE_KEY_RELEASED) {
    //     u16 key_code = context.data.u16[0];
    //     if (key_code == KEY_B) {
    //         // Example on checking for a key
    //         KDEBUG("Explicit - B key released!");
    //     } else {
    //         // KTRACE("'%s' key released in window.", input_keycode_str(key_code));
    //     }
    // }
    return false;
}

b8 application_boot(struct APPLICATION* application_instance) {
    PRINT_INFO("Booting testbed...");

    // Allocate the game state.
    application_instance->state = yallocate_aligned(sizeof(GAME_STATE), 8, MEMORY_TAG_GAME);

    debug_console_create(&((GAME_STATE*)application_instance->state)->debug_console);

    // Setup the frame allocator.
    linear_allocator_create(MEBIBYTES(64), 0, &application_instance->frame_allocator);

    APPLICATION_CONFIG* config = &application_instance->app_config;

    // Configure fonts.
    config->font_config.auto_release = false;
    config->font_config.default_bitmap_font_count = 3;

    BITMAP_FONT_CONFIG* bmp_font_config = yallocate_aligned(sizeof(BITMAP_FONT_CONFIG) * 3, 8, MEMORY_TAG_ARRAY);
    // UbuntuMono21px NotoSans21px
    bmp_font_config[0].name = "Ubuntu Mono 21px";
    bmp_font_config[0].resource_name = "UbuntuMono21px";
    bmp_font_config[0].size = 21;

    bmp_font_config[1].name = "JetBrainsMono";
    bmp_font_config[1].resource_name = "JetBrainsMono";
    bmp_font_config[1].size = 22;

    bmp_font_config[2].name = "NotoSansArabic";
    bmp_font_config[2].resource_name = "NotoSansArabic";
    bmp_font_config[2].size = 40;
    config->font_config.bitmap_font_configs = bmp_font_config;

    SYSTEM_FONT_CONFIG sys_font_config;
    sys_font_config.default_size = 20;
    sys_font_config.name = "Noto Sans";
    sys_font_config.resource_name = "NotoSansCJK";

    config->font_config.default_system_font_count = 1;
    config->font_config.system_font_configs = yallocate_aligned(sizeof(SYSTEM_FONT_CONFIG) * 1, 8, MEMORY_TAG_ARRAY);
    config->font_config.system_font_configs[0] = sys_font_config;

    config->font_config.max_bitmap_font_count = 64;
    config->font_config.max_system_font_count = 64;

    // Configure render views. TODO: read from file?
    if (!configure_render_views(config)) {
        PRINT_ERROR("Failed to configure renderer views. Aborting application.");
        return false;
    }

    // Keymaps
    game_setup_keymaps(application_instance);
    // Console commands
    game_setup_commands(application_instance);

    return true;
}

b8 application_init(APPLICATION* application_instance) {
    PRINT_DEBUG("application_init() called!");

    application_register_events(application_instance);

    debug_console_load(&((GAME_STATE*)application_instance->state)->debug_console);

    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    // TODO: temp load/prepare stuff

    state->models_loaded = false;

    // Create test ui text objects
    if (!ui_text_create(UI_TEXT_TYPE_BITMAP, "Ubuntu Mono 21px", 21, "Some test text 123,\n\tyo!", &state->test_text)) {
        PRINT_ERROR("Failed to load basic ui bitmap text.");
        return false;
    }
    // Move debug text to new bottom of screen.
    ui_text_set_position(&state->test_text, Vector3_create(20, application_instance->app_config.start_height - 75, 0));

    if(!ui_text_create(UI_TEXT_TYPE_BITMAP, "JetBrainsMono", 22, "و الشويه كلاااااام و كلام و الكلام العربى فى سطر لا جديد ههههه", &state->test_sys_text)) {
        PRINT_ERROR("Failed to load basic ui system text.");
        return false;
    }
    ui_text_set_position(&state->test_sys_text, Vector3_create(500, 550, 0));

    // Skybox
    if (!skybox_create("skybox_cube", &state->sb)) {
        PRINT_ERROR("Failed to create skybox, aborting game.");
        return false;
    }

    // World meshes
    // Invalidate all meshes.
    for (u32 i = 0; i < 10; ++i) {
        state->meshes[i].generation = INVALID_ID_U8;
        state->ui_meshes[i].generation = INVALID_ID_U8;
    }

    u8 mesh_count = 0;

    // Load up a cube configuration, and load geometry from it.
    Mesh* cube_mesh = &state->meshes[mesh_count];
    cube_mesh->geometry_count = 1;
    cube_mesh->geometries = yallocate_aligned(sizeof(Mesh*) * cube_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
    GEOMETRY_CONFIG g_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");
    cube_mesh->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh->transform = transform_create();
    mesh_count++;
    cube_mesh->generation = 0;
    cube_mesh->unique_id = identifier_aquire_new_id(cube_mesh);
    // Clean up the allocations for the geometry config.
    geometry_system_config_dispose(&g_config);

    // A second cube
    Mesh* cube_mesh_2 = &state->meshes[mesh_count];
    cube_mesh_2->geometry_count = 1;
    cube_mesh_2->geometries = yallocate_aligned(sizeof(Mesh*) * cube_mesh_2->geometry_count, 8, MEMORY_TAG_ARRAY);
    g_config = geometry_system_generate_cube_config(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube_2", "test_material");
    cube_mesh_2->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh_2->transform = transform_from_position((Vector3){10.0f, 0.0f, 1.0f});
    // Set the first cube as the parent to the second.
    transform_set_parent(&cube_mesh_2->transform, &cube_mesh->transform);
    mesh_count++;
    cube_mesh_2->generation = 0;
    cube_mesh_2->unique_id = identifier_aquire_new_id(cube_mesh_2);
    // Clean up the allocations for the geometry config.
    geometry_system_config_dispose(&g_config);

    // A third cube!
    Mesh* cube_mesh_3 = &state->meshes[mesh_count];
    cube_mesh_3->geometry_count = 1;
    cube_mesh_3->geometries = yallocate_aligned(sizeof(Mesh*) * cube_mesh_3->geometry_count, 8, MEMORY_TAG_ARRAY);
    g_config = geometry_system_generate_cube_config(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube_2", "test_material");
    cube_mesh_3->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh_3->transform = transform_from_position((Vector3){5.0f, 0.0f, 1.0f});
    // Set the second cube as the parent to the third.
    transform_set_parent(&cube_mesh_3->transform, &cube_mesh_2->transform);
    mesh_count++;
    cube_mesh_3->generation = 0;
    cube_mesh_3->unique_id = identifier_aquire_new_id(cube_mesh_3);
    // Clean up the allocations for the geometry config.
    geometry_system_config_dispose(&g_config);

    state->car_mesh = &state->meshes[mesh_count];
    state->car_mesh->unique_id = identifier_aquire_new_id(state->car_mesh);
    state->car_mesh->transform = transform_from_position((Vector3){15.0f, 0.0f, 1.0f});
    mesh_count++;

    state->sponza_mesh = &state->meshes[mesh_count];
    state->sponza_mesh->unique_id = identifier_aquire_new_id(state->sponza_mesh);
    state->sponza_mesh->transform = transform_from_position_rotation_scale((Vector3){15.0f, 0.0f, 1.0f}, Quaternion_identity(), (Vector3){0.05f, 0.05f, 0.05f});
    mesh_count++;

    state->helmet_mesh = &state->meshes[mesh_count];
    state->helmet_mesh->unique_id = identifier_aquire_new_id(state->helmet_mesh);
    state->helmet_mesh->transform = transform_from_position_rotation_scale((Vector3){0.0f, 5.0f, 0.0f}, Quaternion_identity(), (Vector3){10.0f, 10.0f, 10.0f});
    mesh_count++;

    state->duck_mesh = &state->meshes[mesh_count];
    state->duck_mesh->unique_id = identifier_aquire_new_id(state->duck_mesh);
    state->duck_mesh->transform = transform_from_position_rotation_scale((Vector3){0.0f, 15.0f, 0.0f}, Quaternion_identity(), (Vector3){0.025f, 0.025f, 0.025f});
    mesh_count++;

    // Load up some test UI geometry.
    GEOMETRY_CONFIG ui_config;
    ui_config.vertex_size = sizeof(Vertex2D);
    ui_config.vertex_count = 4;
    ui_config.index_size = sizeof(u32);
    ui_config.index_count = 6;
    string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
    string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

    const f32 w = 128.0f;
    const f32 h = 32.0f;
    Vertex2D uiverts[4];
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
    state->ui_meshes[0].unique_id = identifier_aquire_new_id(&state->ui_meshes[0]);
    state->ui_meshes[0].geometry_count = 1;
    state->ui_meshes[0].geometries = yallocate_aligned(sizeof(GEOMETRY*), 8, MEMORY_TAG_ARRAY);
    state->ui_meshes[0].geometries[0] = geometry_system_acquire_from_config(ui_config, true);
    state->ui_meshes[0].transform = transform_create();
    state->ui_meshes[0].generation = 0;

    // Move and rotate it some.
    // Quaternion rotation = Quaternion_from_axis_angle((Vector3){0, 0, 1}, deg_to_rad(-45.0f), false);
    // transform_translate_rotate(&state->ui_meshes[0].transform, (Vector3){5, 5, 0}, rotation);
    transform_translate(&state->ui_meshes[0].transform, (Vector3){650, 5, 0});

    // TODO: end temp load/prepare stuff

    state->world_camera = camera_system_get_default();
    camera_position_set(state->world_camera, (Vector3){10.5f, 5.0f, 9.5f});

    yzero_memory(&application_instance->frame_data, sizeof(APP_FRAME_DATA));

    yzero_memory(&state->update_clock, sizeof(clock));
    yzero_memory(&state->render_clock, sizeof(clock));

    return true;
}

void application_shutdown(APPLICATION* application_instance) {
    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    // TODO: Temp
    skybox_destroy(&state->sb);

    // Destroy ui texts
    ui_text_destroy(&state->test_text);
    ui_text_destroy(&state->test_sys_text);

    debug_console_unload(&state->debug_console);
}

b8 application_update(APPLICATION* application_instance, f32 delta_time) {
    // Ensure this is cleaned up to avoid leaking memory.
    // TODO: Need a version of this that uses the frame allocator.
    if (application_instance->frame_data.world_geometries) {
        darray_destroy(application_instance->frame_data.world_geometries);
        application_instance->frame_data.world_geometries = 0;
    }

    // Reset the frame allocator
    linear_allocator_free_all(&application_instance->frame_allocator);

    // Clear frame data
    yzero_memory(&application_instance->frame_data, sizeof(APP_FRAME_DATA));

    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    clock_start(&state->update_clock);
    state->delta_time = delta_time;

    // Track allocation differences.
    state->prev_alloc_count = state->alloc_count;
    state->alloc_count = get_memory_alloc_count();

    // Perform a small rotation on the first mesh.
    Quaternion rotation = Quaternion_from_axis_angle((Vector3){0, 1, 0}, -0.5f * delta_time, false);
    transform_rotate(&state->meshes[0].transform, rotation);

    // Perform a similar rotation on the second mesh, if it exists.
    transform_rotate(&state->meshes[1].transform, rotation);

    // Perform a similar rotation on the third mesh, if it exists.
    transform_rotate(&state->meshes[2].transform, rotation);

    // Update the bitmap text with camera position. NOTE: just using the default camera for now.
    Camera* world_camera = camera_system_get_default();
    Vector3 pos = camera_position_get(world_camera);
    Vector3 rot = camera_rotation_euler_get(world_camera);

    // Also tack on current mouse state.
    b8 left_down = input_is_button_pressed(BUTTON_LEFT);
    b8 right_down = input_is_button_pressed(BUTTON_RIGHT);
    i32 mouse_x, mouse_y;
    input_get_mouse_position(&mouse_x, &mouse_y);

    // Convert to NDC
    f32 mouse_x_ndc = range_convert_f32((f32)mouse_x, 0.0f, (f32)state->width, -1.0f, 1.0f);
    f32 mouse_y_ndc = range_convert_f32((f32)mouse_y, 0.0f, (f32)state->height, -1.0f, 1.0f);

    f64 fps, frame_time;
    metrics_frame(&fps, &frame_time);

    // Update the frustum
    Vector3 forward = camera_forward(state->world_camera);
    Vector3 right = camera_right(state->world_camera);
    Vector3 up = camera_up(state->world_camera);
    // TODO: get camera fov, aspect, etc.
    state->camera_frustum = frustum_create(&state->world_camera->position, &forward, &right, &up, (f32)state->width / state->height, deg_to_rad(45.0f), 0.1f, 1000.0f);

    // NOTE: starting at a reasonable default to avoid too many reallocs.
    application_instance->frame_data.world_geometries = darray_reserve(GEOMETRY_RENDER_DATA, 512);
    u32 draw_count = 0;
    for (u32 i = 0; i < 10; ++i) {
        Mesh* m = &state->meshes[i];
        if (m->generation != INVALID_ID_U8) {
            Matrice4 model = transform_get_world(&m->transform);

            for (u32 j = 0; j < m->geometry_count; ++j) {
                GEOMETRY* g = m->geometries[j];

                // // Bounding sphere calculation.
                // {
                //     // Translate/scale the extents.
                //     Vector3 extents_min = Vector3_multiply_Matrice4(g->extents.min, model);
                //     Vector3 extents_max = Vector3_multiply_Matrice4(g->extents.max, model);

                //     f32 min = KMIN(KMIN(extents_min.x, extents_min.y), extents_min.z);
                //     f32 max = KMAX(KMAX(extents_max.x, extents_max.y), extents_max.z);
                //     f32 diff = yabs(max - min);
                //     f32 radius = diff * 0.5f;

                //     // Translate/scale the center.
                //     Vector3 center = Vector3_multiply_Matrice4(g->center, model);

                //     if (frustum_intersects_sphere(&state->camera_frustum, &center, radius)) {
                //         // Add it to the list to be rendered.
                //         geometry_render_data data = {0};
                //         data.model = model;
                //         data.geometry = g;
                //         data.unique_id = m->unique_id;
                //         darray_push(application_instance->frame_data.world_geometries, data);

                //         draw_count++;
                //     }
                // }

                // AABB calculation
                {
                    // Translate/scale the extents.
                    // Vector3 extents_min = Vector3_multiply_Matrice4(g->extents.min, model);
                    Vector3 extents_max = Vector3_multiply_Matrice4(g->extents.max, model);

                    // Translate/scale the center.
                    Vector3 center = Vector3_multiply_Matrice4(g->center, model);
                    Vector3 half_extents = {
                        yabs(extents_max.x - center.x),
                        yabs(extents_max.y - center.y),
                        yabs(extents_max.z - center.z),
                    };

                    if (frustum_intersects_aabb(&state->camera_frustum, &center, &half_extents)) {
                        // Add it to the list to be rendered.
                        GEOMETRY_RENDER_DATA data = {0};
                        data.model = model;
                        data.geometry = g;
                        data.unique_id = m->unique_id;
                        darray_push(application_instance->frame_data.world_geometries, data);

                        draw_count++;
                    }
                }
            }
        }
    }

    char* vsync_text = renderer_flag_enabled(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT) ? "YES" : " NO";
    char text_buffer[2048];
    string_format(
        text_buffer,
        "\
FPS: %5.1f(%4.1fms)        Pos=[%7.3f %7.3f %7.3f] Rot=[%7.3f, %7.3f, %7.3f]\n\
Upd: %8.3fus, Rend: %8.3fus Mouse: X=%-5d Y=%-5d   L=%s R=%s   NDC: X=%.6f, Y=%.6f\n\
VSync: %s Drawn: %-5u Hovered: %s%u",
        fps,
        frame_time,
        pos.x, pos.y, pos.z,
        rad_to_deg(rot.x), rad_to_deg(rot.y), rad_to_deg(rot.z),
        state->last_update_elapsed * Y_SEC_TO_US_MULTIPLIER,
        state->render_clock.elapsed * Y_SEC_TO_US_MULTIPLIER,
        mouse_x, mouse_y,
        left_down ? "Y" : "N",
        right_down ? "Y" : "N",
        mouse_x_ndc,
        mouse_y_ndc,
        vsync_text,
        draw_count,
        state->hovered_object_id == INVALID_ID ? "none" : "",
        state->hovered_object_id == INVALID_ID ? 0 : state->hovered_object_id);
    ui_text_set_text(&state->test_text, text_buffer);

    debug_console_update(&((GAME_STATE*)application_instance->state)->debug_console);

    clock_update(&state->update_clock);
    state->last_update_elapsed = state->update_clock.elapsed;

    return true;
}

b8 application_render(APPLICATION* application_instance, struct RENDER_PACKET* packet, f32 delta_time) {
    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    clock_start(&state->render_clock);

    // TODO: temp

    // TODO: Read from frame config.
    packet->view_count = 4;
    packet->views = linear_allocator_allocate(&application_instance->frame_allocator, sizeof(RENDER_VIEW_PACKET) * packet->view_count);

    // Skybox
    SKYBOX_PACKET_DATA skybox_data = {0};
    skybox_data.sb = &state->sb;
    if (!render_view_system_build_packet(render_view_system_get("skybox"), &application_instance->frame_allocator, &skybox_data, &packet->views[0])) {
        PRINT_ERROR("Failed to build packet for view 'skybox'.");
        return false;
    }

    // World
    // TODO: performs a lookup on every frame.
    if (!render_view_system_build_packet(render_view_system_get("world"), &application_instance->frame_allocator, application_instance->frame_data.world_geometries, &packet->views[1])) {
        PRINT_ERROR("Failed to build packet for view 'world_opaque'.");
        return false;
    }

    // ui
    UI_PACKET_DATA ui_packet = {0};

    u32 ui_mesh_count = 0;
    u32 max_ui_meshes = 10;
    Mesh** ui_meshes = linear_allocator_allocate(&application_instance->frame_allocator, sizeof(Mesh*) * max_ui_meshes);

    for (u32 i = 0; i < max_ui_meshes; ++i) {
        if (state->ui_meshes[i].generation != INVALID_ID_U8) {
            ui_meshes[ui_mesh_count] = &state->ui_meshes[i];
            ui_mesh_count++;
        }
    }

    ui_packet.mesh_data.mesh_count = ui_mesh_count;
    ui_packet.mesh_data.meshes = ui_meshes;
    ui_packet.text_count = 2;
    UI_TEXT* debug_console_text = debug_console_get_text(&state->debug_console);
    b8 render_debug_conole = debug_console_text && debug_console_visible(&state->debug_console);
    if (render_debug_conole) {
        ui_packet.text_count += 2;
    }
    UI_TEXT** texts = linear_allocator_allocate(&application_instance->frame_allocator, sizeof(UI_TEXT*) * ui_packet.text_count);
    texts[0] = &state->test_text;
    texts[1] = &state->test_sys_text;
    if (render_debug_conole) {
        texts[2] = debug_console_text;
        texts[3] = debug_console_get_entry_text(&state->debug_console);
    }
    ui_packet.texts = texts;
    if (!render_view_system_build_packet(render_view_system_get("ui"), &application_instance->frame_allocator, &ui_packet, &packet->views[2])) {
        PRINT_ERROR("Failed to build packet for view 'ui'.");
        return false;
    }

    // Pick uses both world and ui packet data.
    PICK_PACKET_DATA pick_packet = {0};
    pick_packet.ui_mesh_data = ui_packet.mesh_data;
    pick_packet.world_mesh_data = application_instance->frame_data.world_geometries;
    pick_packet.texts = ui_packet.texts;
    pick_packet.text_count = ui_packet.text_count;

    if (!render_view_system_build_packet(render_view_system_get("pick"), &application_instance->frame_allocator, &pick_packet, &packet->views[3])) {
        PRINT_ERROR("Failed to build packet for view 'ui'.");
        return false;
    }
    // TODO: end temp

    clock_update(&state->render_clock);

    return true;
}

void application_on_resize(APPLICATION* application_instance, u32 width, u32 height) {
    if (!application_instance->state) {
        return;
    }

    GAME_STATE* state = (GAME_STATE*)application_instance->state;

    state->width = width;
    state->height = height;

    // TODO: temp
    // Move debug text to new bottom of screen.
    ui_text_set_position(&state->test_text, Vector3_create(20, state->height - 75, 0));
    // TODO: end temp
}

void application_lib_on_unload(struct APPLICATION* application_instance) {
    application_unregister_events(application_instance);
    debug_console_on_lib_unload(&((GAME_STATE*)application_instance->state)->debug_console);
    game_remove_commands(application_instance);
    game_remove_keymaps(application_instance);
}

void application_lib_on_load(struct APPLICATION* application_instance) {
    application_register_events(application_instance);
    if ((GAME_STATE*)application_instance->state) {
        debug_console_on_lib_load(&((GAME_STATE*)application_instance->state)->debug_console, application_instance->stage >= APPLICATION_STAGE_BOOT_COMPLETE);
    }
    if (application_instance->stage >= APPLICATION_STAGE_BOOT_COMPLETE) {
        game_setup_commands(application_instance);
        game_setup_keymaps(application_instance);
    }
}

static void toggle_vsync(void) {
    b8 vsync_enabled = renderer_flag_enabled(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT);
    vsync_enabled = !vsync_enabled;
    renderer_flag_set_enabled(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT, vsync_enabled);
}

static b8 application_on_yvar_changed(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT data) {
    if (code == EVENT_CODE_YVAR_CHANGED && strings_equali(data.data.c, "vsync")) {
        toggle_vsync();
    }
    return false;
}

void application_register_events(struct APPLICATION* application_instance) {
    if (application_instance->stage >= APPLICATION_STAGE_BOOT_COMPLETE) {
        // TODO: temp
        event_register(EVENT_CODE_DEBUG0, application_instance, application_on_debug_event);
        event_register(EVENT_CODE_DEBUG1, application_instance, application_on_debug_event);
        event_register(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, application_instance, application_on_event);
        // TODO: end temp

        event_register(EVENT_CODE_KEY_PRESSED, application_instance, application_on_key);
        event_register(EVENT_CODE_KEY_RELEASED, application_instance, application_on_key);

        event_register(EVENT_CODE_YVAR_CHANGED, 0, application_on_yvar_changed);
    }
}

void application_unregister_events(struct APPLICATION* application_instance) {
    event_unregister(EVENT_CODE_DEBUG0, application_instance, application_on_debug_event);
    event_unregister(EVENT_CODE_DEBUG1, application_instance, application_on_debug_event);
    event_unregister(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, application_instance, application_on_event);
    // TODO: end temp

    event_unregister(EVENT_CODE_KEY_PRESSED, application_instance, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, application_instance, application_on_key);

    event_unregister(EVENT_CODE_YVAR_CHANGED, 0, application_on_yvar_changed);
}

b8 configure_render_views(APPLICATION_CONFIG* config) {
    config->render_views = darray_create(RENDER_VIEW_CONFIG);

    // Skybox view
    RENDER_VIEW_CONFIG skybox_config = {0};
    skybox_config.type = RENDERER_VIEW_KNOWN_TYPE_SKYBOX;
    skybox_config.width = 0;
    skybox_config.height = 0;
    skybox_config.name = "skybox";
    skybox_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;

    // Renderpass config.
    skybox_config.passes = darray_create(RENDERPASS_CONFIG);
    RENDERPASS_CONFIG skybox_pass = {0};
    skybox_pass.name = "renderpass.builtin.skybox";
    skybox_pass.render_area = (Vector4){0, 0, (f32)config->start_width, (f32)config->start_height};  // Default render area resolution.
    skybox_pass.clear_color = (Vector4){0.0f, 0.0f, 0.2f, 1.0f};
    skybox_pass.clear_flags = RENDERPASS_CLEAR_COLOR_BUFFER_FLAG;
    skybox_pass.depth = 1.0f;
    skybox_pass.stencil = 0;
    skybox_pass.target.attachments = darray_create(RENDER_TARGET_ATTACHMENT_CONFIG);

    // Color attachment.
    RENDER_TARGET_ATTACHMENT_CONFIG skybox_target_color = {0};
    skybox_target_color.type = RENDER_TARGET_ATTACHMENT_TYPE_COLOR;
    skybox_target_color.source = RENDER_TARGET_ATTACHMENT_SOURCE_DEFAULT;
    skybox_target_color.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_DONT_CARE;
    skybox_target_color.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    skybox_target_color.present_after = false;
    darray_push(skybox_pass.target.attachments, skybox_target_color);

    skybox_pass.target.attachment_count = darray_length(skybox_pass.target.attachments);

    skybox_pass.render_target_count = renderer_window_attachment_count_get();

    darray_push(skybox_config.passes, skybox_pass);
    skybox_config.pass_count = darray_length(skybox_config.passes);

    darray_push(config->render_views, skybox_config);

    // World view.
    RENDER_VIEW_CONFIG world_config = {0};
    world_config.type = RENDERER_VIEW_KNOWN_TYPE_WORLD;
    world_config.width = 0;
    world_config.height = 0;
    world_config.name = "world";
    world_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    world_config.passes = darray_create(RENDERPASS_CONFIG);

    // Renderpass config.
    RENDERPASS_CONFIG world_pass = {0};
    world_pass.name = "renderpass.builtin.world";
    world_pass.render_area = (Vector4){0, 0, (f32)config->start_width, (f32)config->start_height};  // Default render area resolution.
    world_pass.clear_color = (Vector4){0.0f, 0.0f, 0.2f, 1.0f};
    world_pass.clear_flags = RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG;
    world_pass.depth = 1.0f;
    world_pass.stencil = 0;
    world_pass.target.attachments = darray_create(RENDER_TARGET_ATTACHMENT_CONFIG);

    // Colour attachment
    RENDER_TARGET_ATTACHMENT_CONFIG world_target_color = {0};
    world_target_color.type = RENDER_TARGET_ATTACHMENT_TYPE_COLOR;
    world_target_color.source = RENDER_TARGET_ATTACHMENT_SOURCE_DEFAULT;
    world_target_color.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_LOAD;
    world_target_color.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    world_target_color.present_after = false;
    darray_push(world_pass.target.attachments, world_target_color);

    // Depth attachment
    RENDER_TARGET_ATTACHMENT_CONFIG world_target_depth = {0};
    world_target_depth.type = RENDER_TARGET_ATTACHMENT_TYPE_DEPTH;
    world_target_depth.source = RENDER_TARGET_ATTACHMENT_SOURCE_DEFAULT;
    world_target_depth.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_DONT_CARE;
    world_target_depth.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    world_target_depth.present_after = false;
    darray_push(world_pass.target.attachments, world_target_depth);

    world_pass.target.attachment_count = darray_length(world_pass.target.attachments);
    world_pass.render_target_count = renderer_window_attachment_count_get();
    darray_push(world_config.passes, world_pass);

    world_config.pass_count = darray_length(world_config.passes);

    darray_push(config->render_views, world_config);

    // UI view
    RENDER_VIEW_CONFIG ui_view_config = {0};
    ui_view_config.type = RENDERER_VIEW_KNOWN_TYPE_UI;
    ui_view_config.width = 0;
    ui_view_config.height = 0;
    ui_view_config.name = "ui";
    ui_view_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    ui_view_config.passes = darray_create(RENDERPASS_CONFIG);

    // Renderpass config
    RENDERPASS_CONFIG ui_pass;
    ui_pass.name = "renderpass.builtin.ui";
    ui_pass.render_area = (Vector4){0, 0, (f32)config->start_width, (f32)config->start_height};
    ui_pass.clear_color = (Vector4){0.0f, 0.0f, 0.2f, 1.0f};
    ui_pass.clear_flags = RENDERPASS_CLEAR_NONE_FLAG;
    ui_pass.depth = 1.0f;
    ui_pass.stencil = 0;
    ui_pass.target.attachments = darray_create(RENDER_TARGET_ATTACHMENT_CONFIG);

    RENDER_TARGET_ATTACHMENT_CONFIG ui_target_attachment = {0};
    // Colour attachment.
    ui_target_attachment.type = RENDER_TARGET_ATTACHMENT_TYPE_COLOR;
    ui_target_attachment.source = RENDER_TARGET_ATTACHMENT_SOURCE_DEFAULT;
    ui_target_attachment.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_LOAD;
    ui_target_attachment.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    ui_target_attachment.present_after = true;

    darray_push(ui_pass.target.attachments, ui_target_attachment);
    ui_pass.target.attachment_count = darray_length(ui_pass.target.attachments);
    ui_pass.render_target_count = renderer_window_attachment_count_get();

    darray_push(ui_view_config.passes, ui_pass);
    ui_view_config.pass_count = darray_length(ui_view_config.passes);

    darray_push(config->render_views, ui_view_config);

    // Pick pass.
    RENDER_VIEW_CONFIG pick_view_config = {0};
    pick_view_config.type = RENDERER_VIEW_KNOWN_TYPE_PICK;
    pick_view_config.width = 0;
    pick_view_config.height = 0;
    pick_view_config.name = "pick";
    pick_view_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;

    pick_view_config.passes = darray_create(RENDERPASS_CONFIG);

    // World pick pass
    RENDERPASS_CONFIG world_pick_pass = {0};
    world_pick_pass.name = "renderpass.builtin.worldpick";
    world_pick_pass.render_area = (Vector4){0, 0, (f32)config->start_width, (f32)config->start_height};
    world_pick_pass.clear_color = (Vector4){1.0f, 1.0f, 1.0f, 1.0f};  // HACK: clearing to white for better visibility// TODO: Clear to black, as 0 is invalid id.
    world_pick_pass.clear_flags = RENDERPASS_CLEAR_COLOR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG;
    world_pick_pass.depth = 1.0f;
    world_pick_pass.stencil = 0;
    world_pick_pass.target.attachments = darray_create(RENDER_TARGET_ATTACHMENT_CONFIG);

    // Colour attachment
    RENDER_TARGET_ATTACHMENT_CONFIG world_pick_pass_color = {0};
    world_pick_pass_color.type = RENDER_TARGET_ATTACHMENT_TYPE_COLOR;
    world_pick_pass_color.source = RENDER_TARGET_ATTACHMENT_SOURCE_VIEW;  // Obtain the attachment from the view.
    world_pick_pass_color.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_DONT_CARE;
    world_pick_pass_color.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    world_pick_pass_color.present_after = false;
    darray_push(world_pick_pass.target.attachments, world_pick_pass_color);

    // Depth attachment
    RENDER_TARGET_ATTACHMENT_CONFIG world_pick_pass_depth = {0};
    world_pick_pass_depth.type = RENDER_TARGET_ATTACHMENT_TYPE_DEPTH;
    world_pick_pass_depth.source = RENDER_TARGET_ATTACHMENT_SOURCE_VIEW;  // Obtain the attachment from the view.
    world_pick_pass_depth.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_DONT_CARE;
    world_pick_pass_depth.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    world_pick_pass_depth.present_after = false;
    darray_push(world_pick_pass.target.attachments, world_pick_pass_depth);

    world_pick_pass.target.attachment_count = darray_length(world_pick_pass.target.attachments);
    world_pick_pass.render_target_count = 1;  // Not triple-buffering this.
    darray_push(pick_view_config.passes, world_pick_pass);

    // UI pick pass
    RENDERPASS_CONFIG ui_pick_pass = {0};
    ui_pick_pass.name = "renderpass.builtin.uipick";
    ui_pick_pass.render_area = (Vector4){0, 0, (f32)config->start_width, (f32)config->start_height};
    ui_pick_pass.clear_color = (Vector4){1.0f, 1.0f, 1.0f, 1.0f};
    ui_pick_pass.clear_flags = RENDERPASS_CLEAR_NONE_FLAG;
    ui_pick_pass.depth = 1.0f;
    ui_pick_pass.stencil = 0;
    ui_pick_pass.target.attachments = darray_create(RENDER_TARGET_ATTACHMENT_CONFIG);

    RENDER_TARGET_ATTACHMENT_CONFIG ui_pick_pass_attachment = {0};
    ui_pick_pass_attachment.type = RENDER_TARGET_ATTACHMENT_TYPE_COLOR;
    // Obtain the attachment from the view.
    ui_pick_pass_attachment.source = RENDER_TARGET_ATTACHMENT_SOURCE_VIEW;
    ui_pick_pass_attachment.load_operation = RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_LOAD;
    // Need to store it so it can be sampled afterward.
    ui_pick_pass_attachment.store_operation = RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE;
    ui_pick_pass_attachment.present_after = false;
    darray_push(ui_pick_pass.target.attachments, ui_pick_pass_attachment);

    ui_pick_pass.target.attachment_count = darray_length(ui_pick_pass.target.attachments);
    ui_pick_pass.render_target_count = 1;  // Not triple-buffering this.
    darray_push(pick_view_config.passes, ui_pick_pass);

    pick_view_config.pass_count = darray_length(pick_view_config.passes);

    darray_push(config->render_views, pick_view_config);

    return true;
}
