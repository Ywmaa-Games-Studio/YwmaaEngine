#pragma once

#include <defines.h>
#include <game_types.h>
#include <math/math_types.h>
#include <systems/camera_system.h>

// TODO: temp
#include <resources/skybox.h>
#include <resources/ui_text.h>

typedef struct GAME_STATE {
    f32 delta_time;
    Camera* world_camera;

    u16 width, height;

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
    // TODO: end temp
} GAME_STATE;

struct RENDER_PACKET;

b8 game_boot(struct GAME* game_instance);

b8 game_init(GAME* game_instance);

b8 game_update(GAME* game_instance, f32 delta_time);

b8 game_render(GAME* game_instance, struct RENDER_PACKET* packet, f32 delta_time);

void game_on_resize(GAME* game_instance, u32 width, u32 height);

void game_shutdown(GAME* game_inst);
