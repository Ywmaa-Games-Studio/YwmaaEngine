#pragma once
// with dependecy on CLAY UI Library

#include "defines.h"
#include "vendor/clay.h"
#include "renderer/renderer_types.inl"
typedef struct UI_SYSTEM_CONFIG {
    u32 max_ui_count;
    u32 max_text_count;
} UI_SYSTEM_CONFIG;

b8 ui_system_init(u64* memory_requirement, void* state, UI_SYSTEM_CONFIG config);
UI_PACKET_DATA ui_system_render_commands(Clay_RenderCommandArray *rcommands, f32 delta_time);
void ui_system_shutdown(void);
