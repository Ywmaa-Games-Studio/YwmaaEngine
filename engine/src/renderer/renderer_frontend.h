#pragma once

#include "renderer_types.inl"

b8 renderer_system_init(u64* memory_requirement, void* state, const char* application_name, E_RENDERER_BACKEND_API renderer_backend_api);
void renderer_system_shutdown(void* state);

void renderer_on_resized(u16 width, u16 height);

b8 renderer_draw_frame(RENDER_PACKET* packet);

// HACK: this should not be exposed outside the engine.
YAPI void renderer_set_view(Matrice4 view);

void renderer_create_texture(const u8* pixels, struct TEXTURE* texture);

void renderer_destroy_texture(struct TEXTURE* texture);

b8 renderer_create_material(struct MATERIAL* material);
void renderer_destroy_material(struct MATERIAL* material);