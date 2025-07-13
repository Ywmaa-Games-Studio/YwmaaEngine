#include "game.h"

#include <core/logger.h>
#include <core/ystring.h>
#include <input/input.h>
#include <core/event.h>

#include <math/ymath.h>
#include <renderer/renderer_types.inl>

b8 game_init(GAME* game_instance) {
    PRINT_DEBUG("game_init() called!");

    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    state->world_camera = camera_system_get_default();
    camera_position_set(state->world_camera, (Vector3){10.5f, 5.0f, 9.5f});

    return true;
}

b8 game_update(GAME* game_instance, f32 delta_time) {
    // PRINT_DEBUG("game_update() called!");

    static u64 alloc_count = 0;
    u64 prev_alloc_count = alloc_count;
    alloc_count = get_memory_alloc_count();
    if (input_is_key_released('M') && input_was_key_pressed('M')) {
        char* usage = get_memory_usage_str();
        PRINT_INFO(usage);
        string_free(usage);
        PRINT_DEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    }
    // TODO: temp
    if (input_is_key_released('T') && input_was_key_pressed('T')) {
        PRINT_DEBUG("Swapping texture!");
        EVENT_CONTEXT context = {};
        event_fire(EVENT_CODE_DEBUG0, game_instance, context);
    }
    // TODO: end temp


    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    // HACK: temp hack to move camera around.
    if (input_is_key_pressed(KEY_LEFT)) {
        camera_yaw(state->world_camera, 1.0f * delta_time);
    }

    if (input_is_key_pressed(KEY_RIGHT)) {
        camera_yaw(state->world_camera, -1.0f * delta_time);
    }

    if (input_is_key_pressed(KEY_UP)) {
        camera_pitch(state->world_camera, 1.0f * delta_time);
    }

    if (input_is_key_pressed(KEY_DOWN)) {
        camera_pitch(state->world_camera, -1.0f * delta_time);
    }

    static const f32 temp_move_speed = 50.0f;

    if (input_is_key_pressed('W')) {
        camera_move_forward(state->world_camera, temp_move_speed * delta_time);
    }

    if (input_is_key_pressed('S')) {
        camera_move_backward(state->world_camera, temp_move_speed * delta_time);
    }

    if (input_is_key_pressed('A')) {
        camera_move_left(state->world_camera, temp_move_speed * delta_time);
    }

    if (input_is_key_pressed('D')) {
        camera_move_right(state->world_camera, temp_move_speed * delta_time);
    }

    if (input_is_key_pressed(KEY_SPACE)) {
        camera_move_up(state->world_camera, temp_move_speed * delta_time);
    }

    if (input_is_key_pressed('X')) {
        camera_move_down(state->world_camera, temp_move_speed * delta_time);
    }

    // TODO: temp
    if (input_is_key_released('P') && input_was_key_pressed('P')) {
        PRINT_DEBUG(
            "Pos:[%.2f, %.2f, %.2f",
            state->world_camera->position.x,
            state->world_camera->position.y,
            state->world_camera->position.z);
    }

    // RENDERER DEBUG FUNCTIONS
    if (input_is_key_released('1') && input_was_key_pressed('1')) {
        EVENT_CONTEXT data = {0};
        data.data.i32[0] = RENDERER_VIEW_MODE_LIGHTING;
        event_fire(EVENT_CODE_SET_RENDER_MODE, game_instance, data);
    }

    if (input_is_key_released('2') && input_was_key_pressed('2')) {
        EVENT_CONTEXT data = {0};
        data.data.i32[0] = RENDERER_VIEW_MODE_NORMALS;
        event_fire(EVENT_CODE_SET_RENDER_MODE, game_instance, data);
    }

    if (input_is_key_released('0') && input_was_key_pressed('0')) {
        EVENT_CONTEXT data = {0};
        data.data.i32[0] = RENDERER_VIEW_MODE_DEFAULT;
        event_fire(EVENT_CODE_SET_RENDER_MODE, game_instance, data);
    }

    // TODO: temp
    // Bind a key to load up some data.
    if (input_is_key_released('L') && input_was_key_pressed('L')) {
        EVENT_CONTEXT context = {0};
        event_fire(EVENT_CODE_DEBUG1, game_instance, context);
    }
    // TODO: end temp

    return true;
}

b8 game_render(GAME* game_instance, f32 delta_time) {
    return true;
}

void game_on_resize(GAME* game_instance, u32 width, u32 height) {
}