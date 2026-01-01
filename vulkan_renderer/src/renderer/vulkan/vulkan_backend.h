#pragma once

#include "vulkan_renderer_plugin_main.h"
#include "resources/resource_types.h"

struct SHADER;
struct SHADER_UNIFORM;
struct FRAME_DATA;

b8 vulkan_renderer_backend_init(RENDERER_PLUGIN* backend, const RENDERER_BACKEND_CONFIG* config, u8* out_window_render_target_count);
void vulkan_renderer_backend_shutdown(RENDERER_PLUGIN* backend);

void vulkan_renderer_backend_on_resized(RENDERER_PLUGIN* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(RENDERER_PLUGIN* backend, const struct FRAME_DATA* p_frame_data);
b8 vulkan_renderer_backend_end_frame(RENDERER_PLUGIN* backend, const struct FRAME_DATA* p_frame_data);
void vulkan_renderer_viewport_set(RENDERER_PLUGIN* backend, Vector4 rect);
void vulkan_renderer_viewport_reset(RENDERER_PLUGIN* backend);
void vulkan_renderer_scissor_set(RENDERER_PLUGIN* backend, Vector4 rect);
void vulkan_renderer_scissor_reset(RENDERER_PLUGIN* backend);
b8 vulkan_renderer_renderpass_begin(RENDERER_PLUGIN* plugin, RENDERPASS* pass, RENDER_TARGET* target);
b8 vulkan_renderer_renderpass_end(RENDERER_PLUGIN* plugin, RENDERPASS* pass);

void vulkan_renderer_draw_geometry(RENDERER_PLUGIN* backend, GEOMETRY_RENDER_DATA* data);

void vulkan_renderer_texture_create(RENDERER_PLUGIN* backend, const u8* pixels, TEXTURE* texture);
void vulkan_renderer_texture_destroy(RENDERER_PLUGIN* backend, TEXTURE* texture);
void vulkan_renderer_texture_create_writeable(RENDERER_PLUGIN* backend, TEXTURE* t);
void vulkan_renderer_texture_resize(RENDERER_PLUGIN* backend, TEXTURE* t, u32 new_width, u32 new_height);
void vulkan_renderer_texture_write_data(RENDERER_PLUGIN* backend, TEXTURE* t, u32 offset, u32 size, const u8* pixels);
void vulkan_renderer_texture_read_data(RENDERER_PLUGIN* backend, TEXTURE* t, u32 offset, u32 size, void** out_memory);
void vulkan_renderer_texture_read_pixel(RENDERER_PLUGIN* backend, TEXTURE* t, u32 x, u32 y, u8** out_rgba);

b8 vulkan_renderer_create_geometry(RENDERER_PLUGIN* backend, GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void vulkan_renderer_destroy_geometry(RENDERER_PLUGIN* backend, GEOMETRY* geometry);

b8 vulkan_renderer_shader_create(RENDERER_PLUGIN* backend, struct SHADER* shader, const SHADER_CONFIG* config, RENDERPASS* pass, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);
void vulkan_renderer_shader_destroy(RENDERER_PLUGIN* backend, struct SHADER* shader);

b8 vulkan_renderer_shader_init(RENDERER_PLUGIN* backend, struct SHADER* shader);
b8 vulkan_renderer_shader_use(RENDERER_PLUGIN* backend, struct SHADER* shader);
b8 vulkan_renderer_shader_bind_globals(RENDERER_PLUGIN* backend, struct SHADER* s);
b8 vulkan_renderer_shader_bind_instance(RENDERER_PLUGIN* backend, struct SHADER* s, u32 instance_id);
b8 vulkan_renderer_shader_apply_globals(RENDERER_PLUGIN* backend, struct SHADER* s);
b8 vulkan_renderer_shader_apply_instance(RENDERER_PLUGIN* backend, struct SHADER* s, b8 needs_update);
b8 vulkan_renderer_shader_acquire_instance_resources(RENDERER_PLUGIN* backend, struct SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id);
b8 vulkan_renderer_shader_release_instance_resources(RENDERER_PLUGIN* backend, struct SHADER* s, u32 instance_id);
b8 vulkan_renderer_set_uniform(RENDERER_PLUGIN* backend, struct SHADER* frontend_shader, struct SHADER_UNIFORM* uniform, const void* value);
b8 vulkan_shader_after_renderpass(RENDERER_PLUGIN* backend, struct SHADER *shader);

b8 vulkan_renderer_texture_map_acquire_resources(RENDERER_PLUGIN* backend, TEXTURE_MAP* map);
void vulkan_renderer_texture_map_release_resources(RENDERER_PLUGIN* backend, TEXTURE_MAP* map);

b8 vulkan_renderpass_create(RENDERER_PLUGIN* backend, const RENDERPASS_CONFIG* config, RENDERPASS* out_renderpass);
void vulkan_renderpass_destroy(RENDERER_PLUGIN* backend, RENDERPASS* pass);

b8 vulkan_renderer_render_target_create(RENDERER_PLUGIN* backend, u8 attachment_count, RENDER_TARGET_ATTACHMENT* attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target);
void vulkan_renderer_render_target_destroy(RENDERER_PLUGIN* backend, RENDER_TARGET* target, b8 free_internal_memory);

TEXTURE* vulkan_renderer_window_attachment_get(RENDERER_PLUGIN* backend, u8 index);
TEXTURE* vulkan_renderer_depth_attachment_get(RENDERER_PLUGIN* backend, u8 index);
u8 vulkan_renderer_window_attachment_index_get(RENDERER_PLUGIN* backend);
u8 vulkan_renderer_window_attachment_count_get(RENDERER_PLUGIN* backend);

b8 vulkan_renderer_is_multithreaded(RENDERER_PLUGIN* backend);
b8 vulkan_renderer_flag_enabled(RENDERER_PLUGIN* backend, RENDERER_CONFIG_FLAGS flag);
void vulkan_renderer_flag_set_enabled(RENDERER_PLUGIN* backend, RENDERER_CONFIG_FLAGS flag, b8 enabled);

b8 vulkan_buffer_create_internal(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer);
void vulkan_buffer_destroy_internal(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer);
b8 vulkan_buffer_resize(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 new_size);
b8 vulkan_buffer_bind(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset);
b8 vulkan_buffer_unbind(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer);
void* vulkan_buffer_map_memory(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset, u64 size);
void vulkan_buffer_unmap_memory(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset, u64 size);
b8 vulkan_buffer_flush(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset, u64 size);
b8 vulkan_buffer_read(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset, u64 size, void** out_memory);
b8 vulkan_buffer_load_range(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset, u64 size, const void* data);
b8 vulkan_buffer_copy_range(RENDERER_PLUGIN* backend, RENDER_BUFFER* source, u64 source_offset, RENDER_BUFFER* dest, u64 dest_offset, u64 size);
b8 vulkan_buffer_draw(RENDERER_PLUGIN* backend, RENDER_BUFFER* buffer, u64 offset, u32 element_count, b8 bind_only);
