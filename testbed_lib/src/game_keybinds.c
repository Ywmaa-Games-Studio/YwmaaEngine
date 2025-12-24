#include "game_keybinds.h"
#include "game_state.h"

#include <core/event.h>
#include <core/logger.h>
#include <core/ymemory.h>
#include <core/ystring.h>
#include <core/yvar.h>
#include <core/console.h>
#include <renderer/renderer_frontend.h>
#include "debug_console.h"

void game_on_escape_callback(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    PRINT_DEBUG("game_on_escape_callback");
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (EVENT_CONTEXT){0});
}

void game_on_yaw(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    f32 f = 0.0f;
    if (key == KEY_LEFT || key == KEY_A) {
        f = 1.0f;
    } else if (key == KEY_RIGHT || key == KEY_D) {
        f = -1.0f;
    }
    camera_yaw(state->world_camera, f * state->delta_time);
}

void game_on_pitch(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    f32 f = 0.0f;
    if (key == KEY_UP) {
        f = 1.0f;
    } else if (key == KEY_DOWN) {
        f = -1.0f;
    }
    camera_pitch(state->world_camera, f * state->delta_time);
}

void game_on_move_forward(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;
    static const f32 temp_move_speed = 50.0f;
    camera_move_forward(state->world_camera, temp_move_speed * state->delta_time);
}

void game_on_move_backward(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;
    static const f32 temp_move_speed = 50.0f;
    camera_move_backward(state->world_camera, temp_move_speed * state->delta_time);
}

void game_on_move_left(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;
    static const f32 temp_move_speed = 50.0f;
    camera_move_left(state->world_camera, temp_move_speed * state->delta_time);
}

void game_on_move_right(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;
    static const f32 temp_move_speed = 50.0f;
    camera_move_right(state->world_camera, temp_move_speed * state->delta_time);
}

void game_on_move_up(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;
    static const f32 temp_move_speed = 50.0f;
    camera_move_up(state->world_camera, temp_move_speed * state->delta_time);
}

void game_on_move_down(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;
    static const f32 temp_move_speed = 50.0f;
    camera_move_down(state->world_camera, temp_move_speed * state->delta_time);
}

void game_on_console_change_visibility(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    b8 console_visible = debug_console_visible(&state->debug_console);
    console_visible = !console_visible;

    debug_console_visible_set(&state->debug_console, console_visible);
    if (console_visible) {
        input_keymap_push(&state->console_keymap);
    } else {
        input_keymap_pop();
    }
}

void game_on_set_render_mode_default(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    EVENT_CONTEXT data = {0};
    data.data.i32[0] = RENDERER_VIEW_MODE_DEFAULT;
    event_fire(EVENT_CODE_SET_RENDER_MODE, (APPLICATION*)user_data, data);
}

void game_on_set_render_mode_lighting(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    EVENT_CONTEXT data = {0};
    data.data.i32[0] = RENDERER_VIEW_MODE_LIGHTING;
    event_fire(EVENT_CODE_SET_RENDER_MODE, (APPLICATION*)user_data, data);
}

void game_on_set_render_mode_normals(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    EVENT_CONTEXT data = {0};
    data.data.i32[0] = RENDERER_VIEW_MODE_NORMALS;
    event_fire(EVENT_CODE_SET_RENDER_MODE, (APPLICATION*)user_data, data);
}

void game_on_load_scene(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    event_fire(EVENT_CODE_DEBUG1, (APPLICATION*)user_data, (EVENT_CONTEXT){0});
}

void game_on_console_scroll(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    if (key == KEY_PAGEUP) {
        debug_console_move_up(user_data);
    } else if (key == KEY_PAGEDOWN) {
        debug_console_move_down(user_data);
    }
}

void game_on_console_scroll_hold(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    GAME_STATE* state = (GAME_STATE*)user_data;

    static f32 accumulated_time = 0.0f;
    accumulated_time += state->delta_time;
    if (accumulated_time >= 0.1f) {
        if (key == KEY_PAGEUP) {
            debug_console_move_up(user_data);
        } else if (key == KEY_PAGEDOWN) {
            debug_console_move_down(user_data);
        }
        accumulated_time = 0.0f;
    }
}

void game_on_console_history_back(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    debug_console_history_back(user_data);
}

void game_on_console_history_forward(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    debug_console_history_forward(user_data);
}

void game_on_debug_texture_swap(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    PRINT_DEBUG("Swapping texture!");
    EVENT_CONTEXT context = {0};
    event_fire(EVENT_CODE_DEBUG0, user_data, context);
}

void game_on_debug_cam_position(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    PRINT_DEBUG(
        "Pos:[%.2f, %.2f, %.2f",
        state->world_camera->position.x,
        state->world_camera->position.y,
        state->world_camera->position.z);
}

void game_on_debug_vsync_toggle(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    char cmd[30];
    string_ncopy(cmd, "yvar_set_int vsync 0", 29);
    b8 vsync_enabled = renderer_flag_enabled(RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT);
    u32 length = string_length(cmd);
    cmd[length - 1] = vsync_enabled ? '1' : '0';
    console_execute_command(cmd);
}

void game_print_memory_metrics(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    APPLICATION* game_instance = (APPLICATION*)user_data;
    GAME_STATE* state = (GAME_STATE*)game_instance->state;

    char* usage = get_memory_usage_str();
    PRINT_INFO(usage);
    string_free(usage);
    PRINT_DEBUG("Allocations: %llu (%llu this frame)", state->alloc_count, state->alloc_count - state->prev_alloc_count);
}

void game_on_debug_console_history_forward(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    debug_console_history_forward(user_data);
}

void game_on_debug_console_history_back(E_KEYS key, E_KEYMAP_ENTRY_BIND_TYPE type, KEYMAP_MODIFIER modifiers, void* user_data) {
    debug_console_history_back(user_data);
}

void game_setup_keymaps(APPLICATION* game_instance) {

    // Global KEYMAP
    KEYMAP global_keymap = keymap_create();
    keymap_binding_add(&global_keymap, KEY_ESCAPE, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_escape_callback);

    input_keymap_push(&global_keymap);

    // Testbed KEYMAP
    KEYMAP testbed_keymap = keymap_create();
    keymap_binding_add(&testbed_keymap, KEY_A, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_yaw);
    keymap_binding_add(&testbed_keymap, KEY_LEFT, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_yaw);

    keymap_binding_add(&testbed_keymap, KEY_D, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_yaw);
    keymap_binding_add(&testbed_keymap, KEY_RIGHT, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_yaw);

    keymap_binding_add(&testbed_keymap, KEY_UP, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_pitch);
    keymap_binding_add(&testbed_keymap, KEY_DOWN, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_pitch);

    keymap_binding_add(&testbed_keymap, KEY_GRAVE, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_console_change_visibility);

    keymap_binding_add(&testbed_keymap, KEY_W, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_move_forward);
    keymap_binding_add(&testbed_keymap, KEY_S, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_move_backward);
    keymap_binding_add(&testbed_keymap, KEY_Q, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_move_left);
    keymap_binding_add(&testbed_keymap, KEY_E, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_move_right);
    keymap_binding_add(&testbed_keymap, KEY_SPACE, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_move_up);
    keymap_binding_add(&testbed_keymap, KEY_X, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_move_down);

    keymap_binding_add(&testbed_keymap, KEY_0, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_set_render_mode_default);
    keymap_binding_add(&testbed_keymap, KEY_1, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_set_render_mode_lighting);
    keymap_binding_add(&testbed_keymap, KEY_2, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_set_render_mode_normals);

    keymap_binding_add(&testbed_keymap, KEY_L, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_load_scene);

    keymap_binding_add(&testbed_keymap, KEY_T, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_debug_texture_swap);
    keymap_binding_add(&testbed_keymap, KEY_P, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_debug_cam_position);
    keymap_binding_add(&testbed_keymap, KEY_V, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_debug_vsync_toggle);
    keymap_binding_add(&testbed_keymap, KEY_M, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_print_memory_metrics);

    input_keymap_push(&testbed_keymap);

    // A console-specific KEYMAP. Is not pushed by default.
    GAME_STATE* state = ((GAME_STATE*)game_instance->state);
    state->console_keymap = keymap_create();
    state->console_keymap.overrides_all = true;
    keymap_binding_add(&state->console_keymap, KEY_GRAVE, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_console_change_visibility);
    keymap_binding_add(&state->console_keymap, KEY_ESCAPE, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, game_instance, game_on_console_change_visibility);

    keymap_binding_add(&state->console_keymap, KEY_PAGEUP, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, &state->debug_console, game_on_console_scroll);
    keymap_binding_add(&state->console_keymap, KEY_PAGEDOWN, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, &state->debug_console, game_on_console_scroll);
    keymap_binding_add(&state->console_keymap, KEY_PAGEUP, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, &state->debug_console, game_on_console_scroll_hold);
    keymap_binding_add(&state->console_keymap, KEY_PAGEDOWN, KEYMAP_BIND_TYPE_HOLD, KEYMAP_MODIFIER_NONE_BIT, &state->debug_console, game_on_console_scroll_hold);

    keymap_binding_add(&state->console_keymap, KEY_UP, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, &state->debug_console, game_on_console_history_back);
    keymap_binding_add(&state->console_keymap, KEY_DOWN, KEYMAP_BIND_TYPE_PRESS, KEYMAP_MODIFIER_NONE_BIT, &state->debug_console, game_on_console_history_forward);

    // If this was done with the console open, push its keymap.
    b8 console_visible = debug_console_visible(&state->debug_console);
    if (console_visible) {
        input_keymap_push(&state->console_keymap);
    }
}

void game_remove_keymaps(APPLICATION* game_instance) {
    // Pop all keymaps
    while (input_keymap_pop()) {
    }

    GAME_STATE* state = ((GAME_STATE*)game_instance->state);

    // Remove all bindings for the console keymap, since that's the only one we hold onto.
    keymap_clear(&state->console_keymap);
}
