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
#include <resources/simple_scene.h>

#include "debug_console.h"

typedef struct TESTBED_GAME_STATE {
    f32 delta_time;
    Camera* world_camera;

    u16 width, height;

    Frustum camera_frustum;

    native_clock update_clock;
    native_clock render_clock;
    f64 last_update_elapsed;

    // TODO: temp
    SIMPLE_SCENE main_scene;
    b8 main_scene_unload_triggered;

    Mesh meshes[10];

    POINT_LIGHT* p_light_1;

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
    i32 dummy;
} TESTBED_APPLICATION_FRAME_DATA;
