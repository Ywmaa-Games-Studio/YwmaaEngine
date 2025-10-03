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
void vulkan_renderer_viewport_set(Vector4 rect);
void vulkan_renderer_viewport_reset(void);
void vulkan_renderer_scissor_set(Vector4 rect);
void vulkan_renderer_scissor_reset(void);
b8 vulkan_renderer_renderpass_begin(RENDERPASS* pass, RENDER_TARGET* target);
b8 vulkan_renderer_renderpass_end(RENDERPASS* pass);

void vulkan_renderer_draw_geometry(GEOMETRY_RENDER_DATA* data);

void vulkan_renderer_texture_create(const u8* pixels, TEXTURE* texture);
void vulkan_renderer_texture_destroy(TEXTURE* texture);
void vulkan_renderer_texture_create_writeable(TEXTURE* t);
void vulkan_renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height);
void vulkan_renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels);
void vulkan_renderer_texture_read_data(TEXTURE* t, u32 offset, u32 size, void** out_memory);
void vulkan_renderer_texture_read_pixel(TEXTURE* t, u32 x, u32 y, u8** out_rgba);

b8 vulkan_renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void vulkan_renderer_destroy_geometry(GEOMETRY* geometry);

b8 vulkan_renderer_shader_create(struct SHADER* shader, const SHADER_CONFIG* config, RENDERPASS* pass, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);
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

b8 vulkan_renderpass_create(const RENDERPASS_CONFIG* config, RENDERPASS* out_renderpass);
void vulkan_renderpass_destroy(RENDERPASS* pass);

b8 vulkan_renderer_render_target_create(u8 attachment_count, RENDER_TARGET_ATTACHMENT* attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target);
void vulkan_renderer_render_target_destroy(RENDER_TARGET* target, b8 free_internal_memory);

TEXTURE* vulkan_renderer_window_attachment_get(u8 index);
TEXTURE* vulkan_renderer_depth_attachment_get(u8 index);
u8 vulkan_renderer_window_attachment_index_get(void);
u8 vulkan_renderer_window_attachment_count_get(void);

b8 vulkan_renderer_is_multithreaded(void);
b8 vulkan_renderer_flag_enabled(RENDERER_CONFIG_FLAGS flag);
void vulkan_renderer_flag_set_enabled(RENDERER_CONFIG_FLAGS flag, b8 enabled);

b8 vulkan_buffer_create_internal(RENDER_BUFFER* buffer);
void vulkan_buffer_destroy_internal(RENDER_BUFFER* buffer);
b8 vulkan_buffer_resize(RENDER_BUFFER* buffer, u64 new_size);
b8 vulkan_buffer_bind(RENDER_BUFFER* buffer, u64 offset);
b8 vulkan_buffer_unbind(RENDER_BUFFER* buffer);
void* vulkan_buffer_map_memory(RENDER_BUFFER* buffer, u64 offset, u64 size);
void vulkan_buffer_unmap_memory(RENDER_BUFFER* buffer, u64 offset, u64 size);
b8 vulkan_buffer_flush(RENDER_BUFFER* buffer, u64 offset, u64 size);
b8 vulkan_buffer_read(RENDER_BUFFER* buffer, u64 offset, u64 size, void** out_memory);
b8 vulkan_buffer_load_range(RENDER_BUFFER* buffer, u64 offset, u64 size, const void* data);
b8 vulkan_buffer_copy_range(RENDER_BUFFER* source, u64 source_offset, RENDER_BUFFER* dest, u64 dest_offset, u64 size);
b8 vulkan_buffer_draw(RENDER_BUFFER* buffer, u64 offset, u32 element_count, b8 bind_only);
