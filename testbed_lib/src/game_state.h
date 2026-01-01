#pragma once

#include <defines.h>
#include <application_types.h>
#include <math/math_types.h>
#include <systems/camera_system.h>

// TODO: temp
#include <resources/skybox.h>
#include <resources/ui_text.h>
#include <core/clock.h>
#include <input/keymap.h>
#include <systems/light_system.h>

#include "debug_console.h"

typedef struct TESTBED_GAME_STATE {
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

    DIRECTIONAL_LIGHT dir_light;
    POINT_LIGHT p_lights[3];

    Mesh ui_meshes[10];
    UI_TEXT test_text;
    UI_TEXT test_sys_text;

    DEBUG_CONSOLE_STATE debug_console;

    // The unique identifier of the currently hovered-over object.
    u32 hovered_object_id;

    KEYMAP console_keymap;

    u64 alloc_count;
    u64 prev_alloc_count;
    // TODO: end temp
} TESTBED_GAME_STATE;

typedef struct TESTBED_APPLICATION_FRAME_DATA {
    GEOMETRY_RENDER_DATA* world_geometries;
} TESTBED_APPLICATION_FRAME_DATA;
