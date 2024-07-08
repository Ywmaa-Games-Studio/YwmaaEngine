#pragma once

#include "renderer_types.inl"

struct STATIC_MESH_DATA;
struct PLATFORM_STATE;

b8 renderer_init(const char* application_name, struct PLATFORM_STATE* platform_state);
void renderer_shutdown();

void renderer_on_resized(u16 width, u16 height);

b8 renderer_draw_frame(RENDER_PACKET* packet);