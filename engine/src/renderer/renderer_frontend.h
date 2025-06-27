#pragma once

#include "renderer_types.inl"

struct SHADER;
struct SHADER_UNIFORM;

b8 renderer_system_init(u64* memory_requirement, void* state, const char* application_name, E_RENDERER_BACKEND_API renderer_backend_api);
void renderer_system_shutdown(void* state);

void renderer_on_resized(u16 width, u16 height);

b8 renderer_draw_frame(RENDER_PACKET* packet);

/**More actions
 * @brief Sets the view matrix in the renderer. NOTE: exposed to public API.
 *
 * @deprecated HACK: this should not be exposed outside the engine.
 * @param view The view matrix to be set.
 * @param view_position The view position to be set.
 */
YAPI void renderer_set_view(Matrice4 view, Vector3 view_position);

void renderer_texture_create(const u8* pixels, struct TEXTURE* texture);
void renderer_texture_destroy(struct TEXTURE* texture);

/**
 * @brief Creates a new writeable texture with no data written to it.
 *
 * @param t A pointer to the texture to hold the resources.
 */
void renderer_texture_create_writeable(TEXTURE* t);

/**
 * @brief Resizes a texture. There is no check at this level to see if the
 * texture is writeable. Internal resources are destroyed and re-created at
 * the new resolution. Data is lost and would need to be reloaded.
 * 
 * @param t A pointer to the texture to be resized.
 * @param new_width The new width in pixels.
 * @param new_height The new height in pixels.
 */
void renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height);

/**
 * @brief Writes the given data to the provided texture.
 *
 * @param t A pointer to the texture to be written to. NOTE: Must be a writeable texture.
 * @param offset The offset in bytes from the beginning of the data to be written.
 * @param size The number of bytes to be written.
 * @param pixels The raw image data to be written.
 */
void renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels);

b8 renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void renderer_destroy_geometry(GEOMETRY* geometry);

/**
 * @brief Obtains the identifier of the renderpass with the given name.
 *
 * @param name The name of the renderpass whose identifier to obtain.
 * @param out_renderpass_id A pointer to hold the renderpass id.
 * @return True if found; otherwise false.
 */
b8 renderer_renderpass_id(const char* name, u8* out_renderpass_id);

/**
 * @brief Creates internal shader resources using the provided parameters.
 * 
 * @param s A pointer to the shader.
 * @param renderpass_id The identifier of the renderpass to be associated with the shader.
 * @param stage_count The total number of stages.
 * @param stage_filenames An array of shader stage filenames to be loaded. Should align with stages array.
 * @param stages A array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
 * @return b8 True on success; otherwise false.
 */
b8 renderer_shader_create(struct SHADER* s, u8 renderpass_id, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);

/**
 * @brief Destroys the given shader and releases any resources held by it.
 * @param s A pointer to the shader to be destroyed.
 */
void renderer_shader_destroy(struct SHADER* s);

/**
 * @brief Initializes a configured shader. Will be automatically destroyed if this step fails.
 * Must be done after vulkan_shader_create().
 *
 * @param s A pointer to the shader to be initialized.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_init(struct SHADER* s);

/**
 * @brief Uses the given shader, activating it for updates to attributes, uniforms and such,
 * and for use in draw calls.
 *
 * @param s A pointer to the shader to be used.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_use(struct SHADER* s);

/**
 * @brief Binds global resources for use and updating.
 *
 * @param s A pointer to the shader whose globals are to be bound.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_bind_globals(struct SHADER* s);

/**
 * @brief Binds instance resources for use and updating.
 *
 * @param s A pointer to the shader whose instance resources are to be bound.
 * @param instance_id The identifier of the instance to be bound.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_bind_instance(struct SHADER* s, u32 instance_id);

/**
 * @brief Applies global data to the uniform buffer.
 *
 * @param s A pointer to the shader to apply the global data for.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_apply_globals(struct SHADER* s);

/**
 * @brief Applies data for the currently bound instance.
 *
 * @param s A pointer to the shader to apply the instance data for.
 * @param needs_update Indicates if the shader uniforms need to be updated or just bound.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_apply_instance(struct SHADER* s, b8 needs_update);

/**
 * @brief Acquires internal instance-level resources and provides an instance id.
 *
 * @param s A pointer to the shader to acquire resources from.
 * @param maps An array of texture map pointers. Must be one per texture in the instance.
 * @param out_instance_id A pointer to hold the new instance identifier.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_acquire_instance_resources(struct SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id);

/**
 * @brief Releases internal instance-level resources for the given instance id.
 *
 * @param s A pointer to the shader to release resources from.
 * @param instance_id The instance identifier whose resources are to be released.
 * @return True on success; otherwise false.
 */
b8 renderer_shader_release_instance_resources(struct SHADER* s, u32 instance_id);

/**
 * @brief Sets the uniform of the given shader to the provided value.
 * 
 * @param s A ponter to the shader.
 * @param uniform A constant pointer to the uniform.
 * @param value A pointer to the value to be set.
 * @return b8 True on success; otherwise false.
 */
b8 renderer_set_uniform(struct SHADER* s, struct SHADER_UNIFORM* uniform, const void* value);

b8 renderer_shader_after_renderpass(struct SHADER* s);

/**
 * @brief Acquires internal resources for the given texture map.
 *
 * @param map A pointer to the texture map to obtain resources for.
 * @return True on success; otherwise false.
 */
b8 renderer_texture_map_acquire_resources(struct TEXTURE_MAP* map);

/**
 * @brief Releases internal resources for the given texture map.
 *
 * @param map A pointer to the texture map to release resources from.
 */
void renderer_texture_map_release_resources(struct TEXTURE_MAP* map);

