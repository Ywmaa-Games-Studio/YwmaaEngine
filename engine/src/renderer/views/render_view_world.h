#pragma once

#include "defines.h"
#include "renderer/renderer_types.inl"

b8 render_view_world_on_create(struct RENDER_VIEW* self);
void render_view_world_on_destroy(struct RENDER_VIEW* self);
void render_view_world_on_resize(struct RENDER_VIEW* self, u32 width, u32 height);
b8 render_view_world_on_build_packet(const struct RENDER_VIEW* self, void* data, struct RENDER_VIEW_PACKET* out_packet);
void render_view_world_on_destroy_packet(const struct RENDER_VIEW* self, struct RENDER_VIEW_PACKET* packet);
b8 render_view_world_on_render(const struct RENDER_VIEW* self, const struct RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index);
