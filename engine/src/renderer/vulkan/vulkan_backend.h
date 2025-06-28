#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

struct SHADER;
struct SHADER_UNIFORM;

b8 vulkan_renderer_backend_init(RENDERER_BACKEND* backend, const RENDERER_BACKEND_CONFIG* config, u8* out_window_render_target_count);
void vulkan_renderer_backend_shutdown(RENDERER_BACKEND* backend);

void vulkan_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time);
b8 vulkan_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time);

b8 vulkan_renderer_renderpass_begin(RENDERPASS* pass, RENDER_TARGET* target);
b8 vulkan_renderer_renderpass_end(RENDERPASS* pass);
RENDERPASS* vulkan_renderer_renderpass_get(const char* name);

void vulkan_renderer_draw_geometry(GEOMETRY_RENDER_DATA* data);

void vulkan_renderer_texture_create(const u8* pixels, TEXTURE* texture);
void vulkan_renderer_texture_destroy(TEXTURE* texture);
void vulkan_renderer_texture_create_writeable(TEXTURE* t);
void vulkan_renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height);
void vulkan_renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels);

b8 vulkan_renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void vulkan_renderer_destroy_geometry(GEOMETRY* geometry);

b8 vulkan_renderer_shader_create(struct SHADER* shader, RENDERPASS* pass, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);
void vulkan_renderer_shader_destroy(struct SHADER* shader);

b8 vulkan_renderer_shader_init(struct SHADER* shader);
b8 vulkan_renderer_shader_use(struct SHADER* shader);
b8 vulkan_renderer_shader_bind_globals(struct SHADER* s);
b8 vulkan_renderer_shader_bind_instance(struct SHADER* s, u32 instance_id);
b8 vulkan_renderer_shader_apply_globals(struct SHADER* s);
b8 vulkan_renderer_shader_apply_instance(struct SHADER* s, b8 needs_update);
b8 vulkan_renderer_shader_acquire_instance_resources(struct SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id);
b8 vulkan_renderer_shader_release_instance_resources(struct SHADER* s, u32 instance_id);
b8 vulkan_renderer_set_uniform(struct SHADER* frontend_shader, struct SHADER_UNIFORM* uniform, const void* value);
b8 vulkan_shader_after_renderpass(struct SHADER *shader);

b8 vulkan_renderer_texture_map_acquire_resources(TEXTURE_MAP* map);
void vulkan_renderer_texture_map_release_resources(TEXTURE_MAP* map);

void vulkan_renderpass_create(RENDERPASS* out_renderpass, f32 depth, u32 stencil, b8 has_prev_pass, b8 has_next_pass);
void vulkan_renderpass_destroy(RENDERPASS* pass);

void vulkan_renderer_render_target_create(u8 attachment_count, TEXTURE** attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target);
void vulkan_renderer_render_target_destroy(RENDER_TARGET* target, b8 free_internal_memory);

TEXTURE* vulkan_renderer_window_attachment_get(u8 index);
TEXTURE* vulkan_renderer_depth_attachment_get(void);
u8 vulkan_renderer_window_attachment_index_get(void);
