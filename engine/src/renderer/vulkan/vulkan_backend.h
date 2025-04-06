#pragma once

#include "renderer/renderer_backend.h"

b8 vulkan_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name);
void vulkan_renderer_backend_shutdown(RENDERER_BACKEND* backend);

void vulkan_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time);
b8 vulkan_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time);

void vulkan_renderer_update_global_state(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode);

void vulkan_backend_update_object(GEOMETRY_RENDER_DATA data);

void vulkan_renderer_create_texture(const char* name, b8 auto_release, i32 width, i32 height, i32 channel_count, const u8* pixels, b8 has_transparency, TEXTURE* out_texture);
void vulkan_renderer_destroy_texture(TEXTURE* texture);