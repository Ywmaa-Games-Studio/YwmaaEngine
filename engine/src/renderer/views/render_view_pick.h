#pragma once

#include "defines.h"
#include "renderer/renderer_types.inl"

struct LINEAR_ALLOCATOR;

b8 render_view_pick_on_create(struct RENDER_VIEW* self);
void render_view_pick_on_destroy(struct RENDER_VIEW* self);
void render_view_pick_on_resize(struct RENDER_VIEW* self, u32 width, u32 height);
b8 render_view_pick_on_packet_build(const struct RENDER_VIEW* self, struct LINEAR_ALLOCATOR* frame_allocator, void* data, struct RENDER_VIEW_PACKET* out_packet);
void render_view_pick_on_packet_destroy(const struct RENDER_VIEW* self, struct RENDER_VIEW_PACKET* packet);
b8 render_view_pick_on_render(const struct RENDER_VIEW* self, const struct RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index);

void render_view_pick_get_matrices(const struct RENDER_VIEW* self, Matrice4* out_view, Matrice4* out_projection);
b8 render_view_pick_attachment_target_regenerate(struct RENDER_VIEW* self, u32 pass_index, struct RENDER_TARGET_ATTACHMENT* attachment);
