#include "game.h"

#include <core/logger.h>
#include <input/input.h>
#include <core/event.h>

#include <math/ymath.h>

// HACK: This should not be available outside the engine.
#include <renderer/renderer_frontend.h>

//extern void renderer_set_view(Matrice4 view);

void recalculate_view_matrix(GAME_STATE* state) {
    if (state->camera_view_dirty) {
        Matrice4 rotation = Matrice4_euler_xyz(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
        Matrice4 translation = Matrice4_translation(state->camera_position);

        state->view = Matrice4_mul(rotation, translation);
        state->view = Matrice4_inverse(state->view);

        state->camera_view_dirty = false;
    }
}

void camera_yaw(GAME_STATE* state, f32 amount) {
    state->camera_euler.y += amount;
    state->camera_view_dirty = true;
}

void camera_pitch(GAME_STATE* state, f32 amount) {
    state->camera_euler.x += amount;

    // Clamp to avoid Gimball lock.
    f32 limit = deg_to_rad(89.0f);
    state->camera_euler.x = YCLAMP(state->camera_euler.x, -limit, limit);

    state->camera_view_dirty = true;
}

b8 game_init(GAME* game_instance) {
    PRINT_DEBUG("game_init() called!");

    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    state->camera_position = (Vector3){10.5f, 5.0f, 9.5f};
    state->camera_euler = Vector3_zero();

    state->view = Matrice4_translation(state->camera_position);
    state->view = Matrice4_inverse(state->view);
    state->camera_view_dirty = true;
    return true;
}

b8 game_update(GAME* game_instance, f32 delta_time) {
    // PRINT_DEBUG("game_update() called!");
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
        camera_yaw(state, 1.0f * delta_time);
    }

    if (input_is_key_pressed(KEY_RIGHT)) {
        camera_yaw(state, -1.0f * delta_time);
    }

    if (input_is_key_pressed(KEY_UP)) {
        camera_pitch(state, 1.0f * delta_time);
    }

    if (input_is_key_pressed(KEY_DOWN)) {
        camera_pitch(state, -1.0f * delta_time);
    }

    f32 temp_move_speed = 50.0f;
    Vector3 velocity = Vector3_zero();

    if (input_is_key_pressed('W')) {
        Vector3 forward = Matrice4_forward(state->view);
        velocity = Vector3_add(velocity, forward);
    }

    if (input_is_key_pressed('S')) {
        Vector3 backward = Matrice4_backward(state->view);
        velocity = Vector3_add(velocity, backward);
    }

    if (input_is_key_pressed('A')) {
        Vector3 left = Matrice4_left(state->view);
        velocity = Vector3_add(velocity, left);
    }

    if (input_is_key_pressed('D')) {
        Vector3 right = Matrice4_right(state->view);
        velocity = Vector3_add(velocity, right);
    }

    if (input_is_key_pressed(KEY_SPACE)) {
        velocity.y += 1.0f;
    }

    if (input_is_key_pressed('X')) {
        velocity.y -= 1.0f;
    }

    Vector3 z = Vector3_zero();
    if (!Vector3_compare(z, velocity, 0.0002f)) {
        // Be sure to normalize the velocity before applying speed.
        Vector3_normalize(&velocity);
        state->camera_position.x += velocity.x * temp_move_speed * delta_time;
        state->camera_position.y += velocity.y * temp_move_speed * delta_time;
        state->camera_position.z += velocity.z * temp_move_speed * delta_time;
        state->camera_view_dirty = true;
    }

    recalculate_view_matrix(state);

    // HACK: This should not be available outside the engine.
    renderer_set_view(state->view, state->camera_position);

    // TODO: temp
    if (input_is_key_released('P') && input_was_key_pressed('P')) {
        PRINT_DEBUG(
            "Pos:[%.2f, %.2f, %.2f",
            state->camera_position.x,
            state->camera_position.y,
            state->camera_position.z);
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
    // TODO: end temp

    return true;
}

b8 game_render(GAME* game_instance, f32 delta_time) {
    return true;
}

void game_on_resize(GAME* game_instance, u32 width, u32 height) {
}