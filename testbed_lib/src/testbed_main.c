#include "testbed_main.h"
#include "game_state.h"

#include <core/logger.h>
#include <core/ystring.h>
#include <input/input.h>
#include <core/event.h>
#include <core/metrics.h>
#include <core/clock.h>
#include <core/console.h>
#include <core/frame_data.h>

#include <data_structures/darray.h>
#include <memory/linear_allocator.h>

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
#include <systems/light_system.h>
#include <resources/simple_scene.h>
#include <systems/resource_system.h>
#include <systems/layers_system.h>
#include "debug_console.h"
#include "game_commands.h"
#include "game_keybinds.h"
// TODO: end temp

b8 configure_render_views(APPLICATION_CONFIG* config);
void application_register_events(struct APPLICATION* application_instance);
void application_unregister_events(struct APPLICATION* application_instance);
static b8 load_main_scene(struct APPLICATION* application_instance);

b8 application_on_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    APPLICATION* application_instance = (APPLICATION*)listener_inst;
    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

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
    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

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
        if (state->main_scene.state < SIMPLE_SCENE_STATE_LOADING) {
            PRINT_DEBUG("Loading main scene...");
            if (!load_main_scene(application_instance)) {
                PRINT_ERROR("Error loading main scene");
            }
        }
        return true;
    } else if (code == EVENT_CODE_DEBUG2) {
        if (state->main_scene.state == SIMPLE_SCENE_STATE_LOADED) {
            PRINT_DEBUG("Unloading scene...");

            simple_scene_unload(&state->main_scene, false);

            PRINT_DEBUG("Done.");
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
    application_instance->state = yallocate_aligned(sizeof(TESTBED_GAME_STATE), 8, MEMORY_TAG_GAME);

    debug_console_create(&((TESTBED_GAME_STATE*)application_instance->state)->debug_console);

    APPLICATION_CONFIG* config = &application_instance->app_config;

    config->frame_allocator_size = MEBIBYTES(64);
    config->app_frame_data_size = sizeof(TESTBED_APPLICATION_FRAME_DATA);

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

void layer3_init(void) {
    PRINT_INFO("LAYER3 init!")
}

void layer3_update(f32 delta_time) {
    //PRINT_INFO("LAYER3 update! delta %f", delta_time)
}

void layer3_render(f32 delta_time) {
    //PRINT_INFO("LAYER3 render! delta %f", delta_time)
}

void layer3_on_event(u16 code, void* sender, EVENT_CONTEXT data) {
    //PRINT_INFO("LAYER3 event! code %d", code)
}


void layer3_destroy(void) {
    PRINT_INFO("LAYER3 destroy!")
}


void layer1_init(void) {
    PRINT_INFO("LAYER1 init!")
}

void layer2_init(void) {
    PRINT_INFO("LAYER2 init!")
}

void layer1_update(f32 delta_time) {
    PRINT_INFO("LAYER1 update! delta %f", delta_time)
    LAYER layer3 = {0};
    layer3.init = layer3_init;
    layer3.update = layer3_update;
    layer3.render = layer3_render;
    layer3.destroy = layer3_destroy;
    layer3.on_event = layer3_on_event;
    layers_system_transition(1, &layer3);
}

void layer2_update(f32 delta_time) {
    //PRINT_INFO("LAYER2 update! delta %f", delta_time)
}


void layer1_destroy(void) {
    PRINT_INFO("LAYER1 destroy!")
}

void layer2_destroy(void) {
    PRINT_INFO("LAYER2 destroy!")
}

b8 application_init(APPLICATION* application_instance) {
    PRINT_DEBUG("application_init() called!");

    application_register_events(application_instance);

    debug_console_load(&((TESTBED_GAME_STATE*)application_instance->state)->debug_console);

    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

    // World meshes
    // Invalidate all meshes.
    for (u32 i = 0; i < 10; ++i) {
        state->meshes[i].generation = INVALID_ID_U8;
        state->ui_meshes[i].generation = INVALID_ID_U8;
    }

    // Create test ui text objects
    if (!ui_text_create(UI_TEXT_TYPE_BITMAP, "Ubuntu Mono 21px", 21, "Some test text 123,\n\tyo!", &state->test_text)) {
        PRINT_ERROR("Failed to load basic ui bitmap text.");
        return false;
    }
    // Move debug text to new bottom of screen.
    ui_text_position_set(&state->test_text, Vector3_create(20, application_instance->app_config.start_height - 75, 0));

    if(!ui_text_create(UI_TEXT_TYPE_BITMAP, "JetBrainsMono", 22, "و الشويه كلاااااام و كلام و الكلام العربى فى سطر لا جديد ههههه", &state->test_sys_text)) {
        PRINT_ERROR("Failed to load basic ui system text.");
        return false;
    }
    ui_text_position_set(&state->test_sys_text, Vector3_create(500, 550, 0));

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

    //yzero_memory(&application_instance->frame_data, sizeof(APP_FRAME_DATA));

    yzero_memory(&state->update_clock, sizeof(native_clock));
    yzero_memory(&state->render_clock, sizeof(native_clock));
    LAYER layer1 = {0};
    LAYER layer2 = {0};
    layer1.init = layer1_init;
    layer1.update = layer1_update;
    layer1.destroy = layer1_destroy;
    layer2.init = layer2_init;
    layer2.update = layer2_update;
    layer2.destroy = layer2_destroy;
    layers_system_push(&layer2);
    layers_system_push(&layer1);

    return true;
}

void application_shutdown(APPLICATION* application_instance) {
    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

    if (state->main_scene.state == SIMPLE_SCENE_STATE_LOADED) {
        PRINT_DEBUG("Unloading scene...");

        simple_scene_unload(&state->main_scene, true);

        PRINT_DEBUG("Done.");
    }

    // TODO: Temp

    // Destroy ui texts
    ui_text_destroy(&state->test_text);
    ui_text_destroy(&state->test_sys_text);

    debug_console_unload(&state->debug_console);
}

b8 application_update(APPLICATION* application_instance, struct FRAME_DATA* p_frame_data) {

    TESTBED_APPLICATION_FRAME_DATA* app_frame_data = (TESTBED_APPLICATION_FRAME_DATA*)p_frame_data->application_frame_data;
    if (!app_frame_data) {
        return true;
    }

    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

    clock_start(&state->update_clock);

    if (state->main_scene.state >= SIMPLE_SCENE_STATE_LOADED) {
        if (!simple_scene_update(&state->main_scene, p_frame_data)) {
            PRINT_WARNING("Failed to update main scene.");
        }

/*         // Perform a small rotation on the first mesh.
        Quaternion rotation = Quaternion_from_axis_angle((Vector3){0, 1, 0}, -0.5f * p_frame_data->delta_time, false);
        transform_rotate(&state->meshes[0].transform, rotation);

        // Perform a similar rotation on the second mesh, if it exists.
        transform_rotate(&state->meshes[1].transform, rotation);

        // Perform a similar rotation on the third mesh, if it exists.
        transform_rotate(&state->meshes[2].transform, rotation); */

        if (state->p_light_1) {
            state->p_light_1->data.color = (Vector4){
                (ysin(p_frame_data->total_time + 0.0f) + 1.0f) * 0.5f,
                (ysin(p_frame_data->total_time + 0.3f) + 1.0f) * 0.5f,
                (ysin(p_frame_data->total_time + 0.6f) + 1.0f) * 0.5f,
                1.0f};
            state->p_light_1->data.position.x = ysin(p_frame_data->total_time);
        }
    }

    // Track allocation differences.
    state->prev_alloc_count = state->alloc_count;
    state->alloc_count = get_memory_alloc_count();

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

    char* vsync_text = renderer_flag_enabled_get(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT) ? "YES" : " NO";
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
        p_frame_data->drawn_mesh_count,
        state->hovered_object_id == INVALID_ID ? "none" : "",
        state->hovered_object_id == INVALID_ID ? 0 : state->hovered_object_id);
    ui_text_text_set(&state->test_text, text_buffer);

    debug_console_update(&((TESTBED_GAME_STATE*)application_instance->state)->debug_console);

    clock_update(&state->update_clock);
    state->last_update_elapsed = state->update_clock.elapsed;

    return true;
}

b8 application_render(APPLICATION* application_instance, struct RENDER_PACKET* packet, struct FRAME_DATA* p_frame_data) {
    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;
    //TESTBED_APPLICATION_FRAME_DATA* app_frame_data = (TESTBED_APPLICATION_FRAME_DATA*)p_frame_data->application_frame_data;

    clock_start(&state->render_clock);

    // TODO: temp

    // TODO: Read from frame config.
    packet->view_count = 4;
    packet->views = linear_allocator_allocate(p_frame_data->frame_allocator, sizeof(RENDER_VIEW_PACKET) * packet->view_count);

    // FIXME: Read this from config
    packet->views[0].view = render_view_system_get("skybox");
    packet->views[1].view = render_view_system_get("world");
    packet->views[2].view = render_view_system_get("ui");
    packet->views[3].view = render_view_system_get("pick");

    // Tell our scene to generate relevant packet data.
    if (state->main_scene.state == SIMPLE_SCENE_STATE_LOADED) {
        if (!simple_scene_populate_render_packet(&state->main_scene, state->world_camera, (f32)state->width / state->height, p_frame_data, packet)) {
            PRINT_ERROR("Failed populare render packet for main scene.");
            return false;
        }

    }

    // ui
    UI_PACKET_DATA ui_packet = {0};

    u32 ui_mesh_count = 0;
    u32 max_ui_meshes = 10;
    Mesh** ui_meshes = linear_allocator_allocate(p_frame_data->frame_allocator, sizeof(Mesh*) * max_ui_meshes);

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
    UI_TEXT** texts = linear_allocator_allocate(p_frame_data->frame_allocator, sizeof(UI_TEXT*) * ui_packet.text_count);
    texts[0] = &state->test_text;
    texts[1] = &state->test_sys_text;
    if (render_debug_conole) {
        texts[2] = debug_console_text;
        texts[3] = debug_console_get_entry_text(&state->debug_console);
    }
    ui_packet.texts = texts;
    if (!render_view_system_packet_build(render_view_system_get("ui"), p_frame_data->frame_allocator, &ui_packet, &packet->views[2])) {
        PRINT_ERROR("Failed to build packet for view 'ui'.");
        return false;
    }

    // Pick uses both world and ui packet data.
    PICK_PACKET_DATA pick_packet = {0};
    pick_packet.ui_mesh_data = ui_packet.mesh_data;
    pick_packet.world_mesh_data = packet->views[1].geometries;  // TODO: non-hardcoded index?
    pick_packet.texts = ui_packet.texts;
    pick_packet.text_count = ui_packet.text_count;

    if (!render_view_system_packet_build(render_view_system_get("pick"), p_frame_data->frame_allocator, &pick_packet, &packet->views[3])) {
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

    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

    state->width = width;
    state->height = height;

    // TODO: temp
    // Move debug text to new bottom of screen.
    ui_text_position_set(&state->test_text, Vector3_create(20, state->height - 75, 0));
    // TODO: end temp
}

void application_lib_on_unload(struct APPLICATION* application_instance) {
    application_unregister_events(application_instance);
    debug_console_on_lib_unload(&((TESTBED_GAME_STATE*)application_instance->state)->debug_console);
    game_remove_commands(application_instance);
    game_remove_keymaps(application_instance);
}

void application_lib_on_load(struct APPLICATION* application_instance) {
    application_register_events(application_instance);
    if ((TESTBED_GAME_STATE*)application_instance->state) {
        debug_console_on_lib_load(&((TESTBED_GAME_STATE*)application_instance->state)->debug_console, application_instance->stage >= APPLICATION_STAGE_BOOT_COMPLETE);
    }
    if (application_instance->stage >= APPLICATION_STAGE_BOOT_COMPLETE) {
        game_setup_commands(application_instance);
        game_setup_keymaps(application_instance);
    }
}

static void toggle_vsync(void) {
    b8 vsync_enabled = renderer_flag_enabled_get(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT);
    vsync_enabled = !vsync_enabled;
    renderer_flag_enabled_set(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT, vsync_enabled);
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
        event_register(EVENT_CODE_DEBUG2, application_instance, application_on_debug_event);
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
    event_unregister(EVENT_CODE_DEBUG2, application_instance, application_on_debug_event);
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

static b8 load_main_scene(struct APPLICATION* application_instance) {
    TESTBED_GAME_STATE* state = (TESTBED_GAME_STATE*)application_instance->state;

    // Load up config file
    // TODO: clean up resource.
    RESOURCE simple_scene_resource;
    if (!resource_system_load("test_scene", RESOURCE_TYPE_SIMPLE_SCENE, 0, &simple_scene_resource)) {
        PRINT_ERROR("Failed to load scene file, check above logs.");
        return false;
    }

    SIMPLE_SCENE_CONFIG* scene_config = (SIMPLE_SCENE_CONFIG*)simple_scene_resource.data;

    // TODO: temp load/prepare stuff
    if (!simple_scene_create(scene_config, &state->main_scene)) {
        PRINT_ERROR("Failed to create main scene");
        return false;
    }

    // Add objects to scene

/*     // Load up a cube configuration, and load geometry from it.
    MESH_CONFIG cube_0_config = {0};
    cube_0_config.geometry_count = 1;
    cube_0_config.g_configs = yallocate(sizeof(GEOMETRY_CONFIG), MEMORY_TAG_ARRAY);
    cube_0_config.g_configs[0] = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube_0", "test_material");

    cube_0_config.name = string_duplicate("test_cube_0");
    cube_0_config.resource_name = string_duplicate("test_cube_0");
    cube_0_config.parent_name = NULL;

    if (!mesh_create(cube_0_config, &state->meshes[0])) {
        PRINT_ERROR("Failed to create mesh for cube 0");
        return false;
    }
    state->meshes[0].transform = transform_create();
    simple_scene_mesh_add(&state->main_scene, "test_cube_0", &state->meshes[0]);

    // Second cube
    MESH_CONFIG cube_1_config = {0};
    cube_1_config.geometry_count = 1;
    cube_1_config.g_configs = yallocate(sizeof(GEOMETRY_CONFIG), MEMORY_TAG_ARRAY);
    cube_1_config.g_configs[0] = geometry_system_generate_cube_config(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube_1", "test_material");

    cube_1_config.name = string_duplicate("test_cube_1");
    cube_1_config.resource_name = string_duplicate("test_cube_1");
    cube_1_config.parent_name = NULL;

    if (!mesh_create(cube_1_config, &state->meshes[1])) {
        PRINT_ERROR("Failed to create mesh for cube 1");
        return false;
    }
    state->meshes[1].transform = transform_from_position((Vector3){10.0f, 0.0f, 1.0f});
    transform_parent_set(&state->meshes[1].transform, &state->meshes[1].transform);

    simple_scene_mesh_add(&state->main_scene, "test_cube_1", &state->meshes[1]);

    // Third cube!
    MESH_CONFIG cube_2_config = {0};
    cube_2_config.geometry_count = 1;
    cube_2_config.g_configs = yallocate(sizeof(GEOMETRY_CONFIG), MEMORY_TAG_ARRAY);
    cube_2_config.g_configs[0] = geometry_system_generate_cube_config(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube_2", "test_material");

    cube_2_config.name = string_duplicate("test_cube_2");
    cube_2_config.resource_name = string_duplicate("test_cube_2");
    cube_2_config.parent_name = NULL;

    if (!mesh_create(cube_2_config, &state->meshes[2])) {
        PRINT_ERROR("Failed to create mesh for cube 2");
        return false;
    }
    state->meshes[2].transform = transform_from_position((Vector3){5.0f, 0.0f, 1.0f});
    transform_parent_set(&state->meshes[2].transform, &state->meshes[2].transform);

    simple_scene_mesh_add(&state->main_scene, "test_cube_2", &state->meshes[2]); */

    // Initialize
    if (!simple_scene_init(&state->main_scene)) {
        PRINT_ERROR("Failed init main scene, aborting game.");
        return false;
    }

    state->p_light_1 = simple_scene_point_light_get(&state->main_scene, "point_light_1");

    // Actually load the scene.
    return simple_scene_load(&state->main_scene);
}
