#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

struct SHADER;
struct SHADER_UNIFORM;

b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name);
void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend);

void webgpu_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height);

b8 webgpu_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time);
b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time);

b8 webgpu_renderer_begin_renderpass(struct RENDERER_BACKEND* backend, u8 renderpass_id);
b8 webgpu_renderer_end_renderpass(struct RENDERER_BACKEND* backend, u8 renderpass_id);

void webgpu_renderer_draw_geometry(GEOMETRY_RENDER_DATA data);

void webgpu_renderer_texture_create(const u8* pixels, TEXTURE* texture);
void webgpu_renderer_texture_destroy(TEXTURE* texture);
/**
 * @brief Creates a new writeable texture with no data written to it.
 * 
 * @param t A pointer to the texture to hold the resources.
 */
void webgpu_renderer_texture_create_writeable(TEXTURE* t);

/**
 * @brief Resizes a texture. There is no check at this level to see if the
 * texture is writeable. Internal resources are destroyed and re-created at
 * the new resolution. Data is lost and would need to be reloaded.
 * 
 * @param t A pointer to the texture to be resized.
 * @param new_width The new width in pixels.
 * @param new_height The new height in pixels.
 */
void webgpu_renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height);

/**
 * @brief Writes the given data to the provided texture.
 * NOTE: At this level, this can either be a writeable or non-writeable texture because
 * this also handles the initial texture load. The texture system itself should be
 * responsible for blocking write requests to non-writeable textures.
 * 
 * @param t A pointer to the texture to be written to. 
 * @param offset The offset in bytes from the beginning of the data to be written.
 * @param size The number of bytes to be written.
 * @param pixels The raw image data to be written.
 */
void webgpu_renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels);


b8 webgpu_renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void webgpu_renderer_destroy_geometry(GEOMETRY* geometry);

b8 webgpu_renderer_shader_create(struct SHADER* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);
void webgpu_renderer_shader_destroy(struct SHADER* s);

b8 webgpu_renderer_shader_init(struct SHADER* shader);
b8 webgpu_renderer_shader_use(struct SHADER* shader);
b8 webgpu_renderer_shader_bind_globals(struct SHADER* s);
b8 webgpu_renderer_shader_bind_instance(struct SHADER* s, u32 instance_id);
b8 webgpu_renderer_shader_apply_globals(struct SHADER* s);
b8 webgpu_renderer_shader_apply_instance(struct SHADER* s, b8 needs_update);
b8 webgpu_renderer_shader_acquire_instance_resources(struct SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id);
b8 webgpu_renderer_shader_release_instance_resources(struct SHADER* s, u32 instance_id);
b8 webgpu_renderer_set_uniform(struct SHADER* frontend_shader, struct SHADER_UNIFORM* uniform, const void* value);
b8 webgpu_shader_after_renderpass(struct SHADER *shader);

b8 webgpu_renderer_texture_map_acquire_resources(TEXTURE_MAP* map);
void webgpu_renderer_texture_map_release_resources(TEXTURE_MAP* map);

