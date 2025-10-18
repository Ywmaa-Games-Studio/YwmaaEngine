#pragma once

#include "defines.h"

struct UI_TEXT;

void debug_console_create(void);

b8 debug_console_load(void);
void debug_console_unload(void);
void debug_console_update(void);

struct UI_TEXT* debug_console_get_text(void);
struct UI_TEXT* debug_console_get_entry_text(void);

b8 debug_console_visible(void);
void debug_console_visible_set(b8 visible);

void debug_console_move_up(void);
void debug_console_move_down(void);
void debug_console_move_to_top(void);
void debug_console_move_to_bottom(void);

void debug_console_history_back(void);
void debug_console_history_forward(void);
