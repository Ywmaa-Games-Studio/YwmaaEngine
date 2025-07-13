#include "input/input.h"
#include "core/event.h"
#include "core/ymemory.h"
#include "core/logger.h"

typedef struct KEYBOARD_STATE {
    b8 keys[256];
} KEYBOARD_STATE;

typedef struct MOUSE_STATE {
    i16 x;
    i16 y;
    b8 buttons[BUTTON_MAX_BUTTONS];
} MOUSE_STATE;

typedef struct INPUT_STATE {
    KEYBOARD_STATE keyboard_current;
    KEYBOARD_STATE keyboard_previous;
    MOUSE_STATE mouse_current;
    MOUSE_STATE mouse_previous;
} INPUT_STATE;

// Internal input state pointer
static INPUT_STATE* state_ptr;

void input_system_init(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(INPUT_STATE);
    if (state == 0) {
        return;
    }
    yzero_memory(state, sizeof(INPUT_STATE));
    state_ptr = state;
    PRINT_INFO("Input subsystem initialized.");
}

void input_system_shutdown(void* state) {
    // TODO: Add shutdown routines when needed.
    state_ptr = 0;
}

void input_update(f64 delta_time) {
    if (!state_ptr) {
        return;
    }

    // Copy current states to previous states.
    ycopy_memory(&state_ptr->keyboard_previous, &state_ptr->keyboard_current, sizeof(KEYBOARD_STATE));
    ycopy_memory(&state_ptr->mouse_previous, &state_ptr->mouse_current, sizeof(MOUSE_STATE));
}

void input_process_key(E_KEYS key, b8 pressed) {
/*     if (key == KEY_LALT) {
        PRINT_INFO("Left alt %s.", pressed ? "pressed" : "released.");
    } else if (key == KEY_RALT) {
        PRINT_INFO("Right alt %s.", pressed ? "pressed" : "released.");
    }

    if (key == KEY_LCONTROL) {
        PRINT_INFO("Left ctrl %s.", pressed ? "pressed" : "released.");
    } else if (key == KEY_RCONTROL) {
        PRINT_INFO("Right ctrl %s.", pressed ? "pressed" : "released.");
    }

    if (key == KEY_LSHIFT) {
        PRINT_INFO("Left shift %s.", pressed ? "pressed" : "released.");
    } else if (key == KEY_RSHIFT) {
        PRINT_INFO("Right shift %s.", pressed ? "pressed" : "released.");
    } */

    // Only handle this if the state actually changed.
    if (state_ptr->keyboard_current.keys[key] != pressed) {
        // Update internal state.
        state_ptr->keyboard_current.keys[key] = pressed;

        // Fire off an event for immediate processing.
        EVENT_CONTEXT context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_button(E_BUTTONS button, b8 pressed) {
    // PRINT_DEBUG("Button %i is pressed", button);
    // If the state changed, fire an event.
    if (state_ptr->mouse_current.buttons[button] != pressed) {
        state_ptr->mouse_current.buttons[button] = pressed;
        // Fire the event.
        EVENT_CONTEXT context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    // Only process if actually different
    if (state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y) {
        // NOTE: Enable this if debugging.
        // PRINT_DEBUG("Mouse pos: %i, %i!", x, y);

        // Update internal state.
        state_ptr->mouse_current.x = x;
        state_ptr->mouse_current.y = y;

        // Fire the event.
        EVENT_CONTEXT context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    // NOTE: no internal state to update.
    // PRINT_DEBUG(z_delta > 0.0 ? "scroll up" : "scroll down");

    // Fire the event.
    EVENT_CONTEXT context;
    context.data.i8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

b8 input_is_key_just_pressed(E_KEYS key) {
    if (!state_ptr) {
        return false;
    }
    return input_is_key_pressed(key) == true && input_was_key_released(key) == true;
}

b8 input_is_key_just_released(E_KEYS key) {
    if (!state_ptr) {
        return true;
    }
    return input_is_key_released(key) == true && input_was_key_pressed(key) == true;
}

b8 input_is_key_pressed(E_KEYS key) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->keyboard_current.keys[key] == true;
}

b8 input_is_key_released(E_KEYS key) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->keyboard_current.keys[key] == false;
}

b8 input_was_key_pressed(E_KEYS key) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->keyboard_previous.keys[key] == true;
}

b8 input_was_key_released(E_KEYS key) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->keyboard_previous.keys[key] == false;
}

// mouse input

b8 input_is_button_just_pressed(E_BUTTONS button) {
    if (!state_ptr) {
        return false;
    }
    return input_is_button_pressed(button) == true && input_was_button_released(button) == true;
}

b8 input_is_button_just_released(E_BUTTONS button) {
    if (!state_ptr) {
        return true;
    }
    return input_is_button_released(button) == true && input_was_button_pressed(button) == true;
}

b8 input_is_button_pressed(E_BUTTONS button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_current.buttons[button] == true;
}

b8 input_is_button_released(E_BUTTONS button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_current.buttons[button] == false;
}

b8 input_was_button_pressed(E_BUTTONS button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_previous.buttons[button] == true;
}

b8 input_was_button_released(E_BUTTONS button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_current.x;
    *y = state_ptr->mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_previous.x;
    *y = state_ptr->mouse_previous.y;
}
