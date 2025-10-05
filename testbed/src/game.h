#pragma once

#include <defines.h>
#include <application_types.h>
#include <math/math_types.h>
#include <systems/camera_system.h>

// TODO: temp
#include <resources/skybox.h>
#include <resources/ui_text.h>
#include <core/clock.h>
#include <core/keymap.h>

typedef struct GAME_STATE {
    f32 delta_time;
    Camera* world_camera;

    u16 width, height;

    Frustum camera_frustum;

    clock update_clock;
    clock render_clock;
    f64 last_update_elapsed;

    // TODO: temp
    Skybox sb;

    Mesh meshes[10];
    Mesh* car_mesh;
    Mesh* sponza_mesh;
    Mesh* helmet_mesh;
    Mesh* duck_mesh;
    b8 models_loaded;

    Mesh ui_meshes[10];
    UI_TEXT test_text;
    UI_TEXT test_sys_text;

    // The unique identifier of the currently hovered-over object.
    u32 hovered_object_id;

    KEYMAP console_keymap;

    u64 alloc_count;
    u64 prev_alloc_count;
    // TODO: end temp
} GAME_STATE;

struct RENDER_PACKET;

b8 game_boot(struct APPLICATION* game_instance);

b8 game_init(APPLICATION* game_instance);

b8 game_update(APPLICATION* game_instance, f32 delta_time);

b8 game_render(APPLICATION* game_instance, struct RENDER_PACKET* packet, f32 delta_time);

void game_on_resize(APPLICATION* game_instance, u32 width, u32 height);

void game_shutdown(APPLICATION* game_inst);
