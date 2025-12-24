#pragma once

#include "defines.h"

#include <resources/ui_text.h>
typedef struct COMMAND_HISTORY_ENTRY {
    const char* command;
} COMMAND_HISTORY_ENTRY;

// TODO: statically-defined state for now.
typedef struct DEBUG_CONSOLE_STATE {
    u8 console_consumer_id;
    // Number of lines displayed at once.
    i32 line_display_count;
    // Number of lines offset from bottom of list.
    i32 line_offset;
    // darray
    char** lines;
    // darray
    COMMAND_HISTORY_ENTRY* history;
    i32 history_offset;

    b8 dirty;
    b8 visible;

    UI_TEXT text_control;
    UI_TEXT entry_control;

} DEBUG_CONSOLE_STATE;

void debug_console_create(DEBUG_CONSOLE_STATE* out_console_state);

b8 debug_console_load(DEBUG_CONSOLE_STATE* state);
void debug_console_unload(DEBUG_CONSOLE_STATE* state);
void debug_console_update(DEBUG_CONSOLE_STATE* state);

void debug_console_on_lib_load(DEBUG_CONSOLE_STATE* state, b8 update_consumer);
void debug_console_on_lib_unload(DEBUG_CONSOLE_STATE* state);

UI_TEXT* debug_console_get_text(DEBUG_CONSOLE_STATE* state);
UI_TEXT* debug_console_get_entry_text(DEBUG_CONSOLE_STATE* state);

b8 debug_console_visible(DEBUG_CONSOLE_STATE* state);
void debug_console_visible_set(DEBUG_CONSOLE_STATE* state, b8 visible);

void debug_console_move_up(DEBUG_CONSOLE_STATE* state);
void debug_console_move_down(DEBUG_CONSOLE_STATE* state);
void debug_console_move_to_top(DEBUG_CONSOLE_STATE* state);
void debug_console_move_to_bottom(DEBUG_CONSOLE_STATE* state);

void debug_console_history_back(DEBUG_CONSOLE_STATE* state);
void debug_console_history_forward(DEBUG_CONSOLE_STATE* state);
