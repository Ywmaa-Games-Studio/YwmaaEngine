#pragma once

#include "webgpu_renderer_plugin_main.h"
#include "resources/resource_types.h"

struct SHADER;
struct SHADER_UNIFORM;

b8 webgpu_renderer_backend_init(RENDERER_PLUGIN* plugin, const RENDERER_BACKEND_CONFIG* config, u8* out_window_render_target_count);
void webgpu_renderer_backend_shutdown(RENDERER_PLUGIN* plugin);

void webgpu_renderer_backend_on_resized(RENDERER_PLUGIN* plugin, u16 width, u16 height);

b8 webgpu_renderer_backend_begin_frame(RENDERER_PLUGIN* plugin, f32 delta_time);
b8 webgpu_renderer_backend_end_frame(RENDERER_PLUGIN* plugin, f32 delta_time);
void webgpu_renderer_viewport_set(RENDERER_PLUGIN* plugin, Vector4 rect);
void webgpu_renderer_viewport_reset(RENDERER_PLUGIN* plugin);
void webgpu_renderer_scissor_set(RENDERER_PLUGIN* plugin, Vector4 rect);
void webgpu_renderer_scissor_reset(RENDERER_PLUGIN* plugin);
b8 webgpu_renderer_renderpass_begin(RENDERER_PLUGIN* plugin, RENDERPASS* pass, RENDER_TARGET* target);
b8 webgpu_renderer_renderpass_end(RENDERER_PLUGIN* plugin, RENDERPASS* pass);
RENDERPASS* webgpu_renderer_renderpass_get(RENDERER_PLUGIN* plugin, const char* name);

void webgpu_renderer_draw_geometry(RENDERER_PLUGIN* plugin, GEOMETRY_RENDER_DATA* data);

void webgpu_renderer_texture_create(RENDERER_PLUGIN* plugin, const u8* pixels, TEXTURE* texture);
void webgpu_renderer_texture_destroy(RENDERER_PLUGIN* plugin, TEXTURE* texture);
void webgpu_renderer_texture_create_writeable(RENDERER_PLUGIN* plugin, TEXTURE* t);
void webgpu_renderer_texture_resize(RENDERER_PLUGIN* plugin, TEXTURE* t, u32 new_width, u32 new_height);
void webgpu_renderer_texture_write_data(RENDERER_PLUGIN* plugin, TEXTURE* t, u32 offset, u32 size, const u8* pixels);
void webgpu_renderer_texture_read_data(RENDERER_PLUGIN* plugin, TEXTURE* t, u32 offset, u32 size, void** out_memory);
void webgpu_renderer_texture_read_pixel(RENDERER_PLUGIN* plugin, TEXTURE* t, u32 x, u32 y, u8** out_rgba);

b8 webgpu_renderer_create_geometry(RENDERER_PLUGIN* plugin, GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void webgpu_renderer_destroy_geometry(RENDERER_PLUGIN* plugin, GEOMETRY* geometry);

b8 webgpu_renderer_shader_create(RENDERER_PLUGIN* plugin, struct SHADER* shader, const SHADER_CONFIG* config, RENDERPASS* pass, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);
void webgpu_renderer_shader_destroy(RENDERER_PLUGIN* plugin, struct SHADER* s);

b8 webgpu_renderer_shader_init(RENDERER_PLUGIN* plugin, struct SHADER* shader);
b8 webgpu_renderer_shader_use(RENDERER_PLUGIN* plugin, struct SHADER* shader);
b8 webgpu_renderer_shader_bind_globals(RENDERER_PLUGIN* plugin, struct SHADER* s);
b8 webgpu_renderer_shader_bind_instance(RENDERER_PLUGIN* plugin, struct SHADER* s, u32 instance_id);
b8 webgpu_renderer_shader_apply_globals(RENDERER_PLUGIN* plugin, struct SHADER* s);
b8 webgpu_renderer_shader_apply_instance(RENDERER_PLUGIN* plugin, struct SHADER* s, b8 needs_update);
b8 webgpu_renderer_shader_acquire_instance_resources(RENDERER_PLUGIN* plugin, struct SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id);
b8 webgpu_renderer_shader_release_instance_resources(RENDERER_PLUGIN* plugin, struct SHADER* s, u32 instance_id);
b8 webgpu_renderer_set_uniform(RENDERER_PLUGIN* plugin, struct SHADER* frontend_shader, struct SHADER_UNIFORM* uniform, const void* value);
b8 webgpu_shader_after_renderpass(RENDERER_PLUGIN* plugin, struct SHADER *shader);

b8 webgpu_renderer_texture_map_acquire_resources(RENDERER_PLUGIN* plugin, TEXTURE_MAP* map);
void webgpu_renderer_texture_map_release_resources(RENDERER_PLUGIN* plugin, TEXTURE_MAP* map);

b8 webgpu_renderpass_create(RENDERER_PLUGIN* plugin, const RENDERPASS_CONFIG* config, RENDERPASS* out_renderpass);
void webgpu_renderpass_destroy(RENDERER_PLUGIN* plugin, RENDERPASS* pass);

b8 webgpu_renderer_render_target_create(RENDERER_PLUGIN* plugin, u8 attachment_count, RENDER_TARGET_ATTACHMENT* attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target);
void webgpu_renderer_render_target_destroy(RENDERER_PLUGIN* plugin, RENDER_TARGET* target, b8 free_internal_memory);

TEXTURE* webgpu_renderer_window_attachment_get(RENDERER_PLUGIN* plugin, u8 index);
TEXTURE* webgpu_renderer_depth_attachment_get(RENDERER_PLUGIN* plugin, u8 index);
u8 webgpu_renderer_window_attachment_index_get(RENDERER_PLUGIN* plugin);
u8 webgpu_renderer_window_attachment_count_get(RENDERER_PLUGIN* plugin);

b8 webgpu_renderer_is_multithreaded(RENDERER_PLUGIN* plugin);
b8 webgpu_renderer_flag_enabled(RENDERER_PLUGIN* plugin, RENDERER_CONFIG_FLAGS flag);
void webgpu_renderer_flag_set_enabled(RENDERER_PLUGIN* plugin, RENDERER_CONFIG_FLAGS flag, b8 enabled);

b8 webgpu_buffer_create_internal(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer);
void webgpu_buffer_destroy_internal(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer);
b8 webgpu_buffer_resize(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 new_size);
b8 webgpu_buffer_bind(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset);
b8 webgpu_buffer_unbind(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer);
void* webgpu_buffer_map_memory(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset, u64 size);
void webgpu_buffer_unmap_memory(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset, u64 size);
b8 webgpu_buffer_flush(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset, u64 size);
b8 webgpu_buffer_read(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset, u64 size, void** out_memory);
b8 webgpu_buffer_load_range(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset, u64 size, const void* data);
b8 webgpu_buffer_copy_range(RENDERER_PLUGIN* plugin, RENDER_BUFFER* source, u64 source_offset, RENDER_BUFFER* dest, u64 dest_offset, u64 size);
b8 webgpu_buffer_draw(RENDERER_PLUGIN* plugin, RENDER_BUFFER* buffer, u64 offset, u32 element_count, b8 bind_only);
