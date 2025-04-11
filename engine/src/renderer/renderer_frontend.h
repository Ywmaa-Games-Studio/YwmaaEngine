#pragma once

#include "renderer_types.inl"

b8 renderer_system_init(u64* memory_requirement, void* state, const char* application_name, E_RENDERER_BACKEND_API renderer_backend_api);
void renderer_system_shutdown(void* state);

void renderer_on_resized(u16 width, u16 height);

b8 renderer_draw_frame(RENDER_PACKET* packet);

// HACK: this should not be exposed outside the engine.
YAPI void renderer_set_view(Matrice4 view);

void renderer_create_texture(
    const char* name,
    i32 width,
    i32 height,
    i32 channel_count,
    const u8* pixels,
    b8 has_transparency,
    TEXTURE* out_texture);

void renderer_destroy_texture(struct TEXTURE* texture);