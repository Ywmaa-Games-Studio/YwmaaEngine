#pragma once

#include "renderer/renderer_backend.h"

b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name);
void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend);

void webgpu_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height);

b8 webgpu_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time);
b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time);

b8 webgpu_renderer_begin_renderpass(struct RENDERER_BACKEND* backend, u8 renderpass_id);
b8 webgpu_renderer_end_renderpass(struct RENDERER_BACKEND* backend, u8 renderpass_id);

void webgpu_renderer_update_global_world_state(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode);
void webgpu_renderer_update_global_ui_state(Matrice4 projection, Matrice4 view, i32 mode);

void webgpu_renderer_draw_geometry(GEOMETRY_RENDER_DATA data);

void webgpu_renderer_create_texture(const u8* pixels, TEXTURE* texture);
void webgpu_renderer_destroy_texture(TEXTURE* texture);

b8 webgpu_renderer_create_material(struct MATERIAL* material);
void webgpu_renderer_destroy_material(struct MATERIAL* material);

b8 webgpu_renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void webgpu_renderer_destroy_geometry(GEOMETRY* geometry);
