#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"
#include "memory/freelist.h"

#define BUILTIN_SHADER_NAME_SKYBOX "shader.builtin.skybox"
#define BUILTIN_SHADER_NAME_MATERIAL "shader.builtin.material"
#define BUILTIN_SHADER_NAME_UI "shader.builtin.ui"

struct SHADER;
struct SHADER_UNIFORM;

typedef enum E_RENDERER_BACKEND_API {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_WEBGPU,
} E_RENDERER_BACKEND_API;

typedef struct MATERIAL_SHADER_GLOBAL_UBO {
    Matrice4 projection;   // 64 bytes
    Matrice4 view;         // 64 bytes
    Matrice4 m_reserved0;  // 64 bytes, reserved for future use
    Matrice4 m_reserved1;  // 64 bytes, reserved for future use
} MATERIAL_SHADER_GLOBAL_UBO;

typedef struct MATERIAL_SHADER_INSTANCE_UBO {
    Vector4 diffuse_color;  // 16 bytes
    Vector4 v_reserved0;    // 16 bytes, reserved for future use
    Vector4 v_reserved1;    // 16 bytes, reserved for future use
    Vector4 v_reserved2;    // 16 bytes, reserved for future use
    // works with vulkan but exceeds the 256 bytes limit in webgpu
//    Matrice4 m_reserved0;    // 64 bytes, reserved for future use
//    Matrice4 m_reserved1;    // 64 bytes, reserved for future use
//    Matrice4 m_reserved2;    // 64 bytes, reserved for future use
} MATERIAL_SHADER_INSTANCE_UBO;

/**
 * @brief uniform buffer object for the ui shader. 
 */
typedef struct UI_SHADER_GLOBAL_UBO {
    Matrice4 projection;   // 64 bytes
    Matrice4 view;         // 64 bytes
    Matrice4 m_reserved0;  // 64 bytes, reserved for future use
    Matrice4 m_reserved1;  // 64 bytes, reserved for future use
} UI_SHADER_GLOBAL_UBO;

/**
 * @brief ui material instance uniform buffer object for the ui shader. 
 */
typedef struct UI_SHADER_INSTANCE_UBO {
    Vector4 diffuse_color;  // 16 bytes
    Vector4 v_reserved0;    // 16 bytes, reserved for future use
    Vector4 v_reserved1;    // 16 bytes, reserved for future use
    Vector4 v_reserved2;    // 16 bytes, reserved for future use
    // works with vulkan but exceeds the 256 bytes limit in webgpu
//    Matrice4 m_reserved0;    // 64 bytes, reserved for future use
//    Matrice4 m_reserved1;    // 64 bytes, reserved for future use
//    Matrice4 m_reserved2;    // 64 bytes, reserved for future use
} UI_SHADER_INSTANCE_UBO;

typedef struct GEOMETRY_RENDER_DATA {
    u32 object_id;
    Matrice4 model;
    GEOMETRY* geometry;
} GEOMETRY_RENDER_DATA;

typedef enum E_RENDERER_DEBUG_VIEW_MODE {
    RENDERER_VIEW_MODE_DEFAULT = 0,
    RENDERER_VIEW_MODE_LIGHTING = 1,
    RENDERER_VIEW_MODE_NORMALS = 2
} E_RENDERER_DEBUG_VIEW_MODE;

/** @brief Represents a render target, which is used for rendering to a texture or set of textures. */
typedef struct RENDER_TARGET {
    /** @brief Indicates if this render target should be updated on window resize. */
    b8 sync_to_window_size;
    /** @brief The number of attachments */
    u8 attachment_count;
    /** @brief An array of attachments (pointers to textures). */
    struct TEXTURE** attachments;
    /** @brief The renderer API internal framebuffer object. */
    void* internal_framebuffer;
} RENDER_TARGET;

/**
 * @brief The types of clearing to be done on a renderpass.
 * Can be combined together for multiple clearing functions.
 */
typedef enum E_RENDERPASS_CLEAR_FLAG {
    /** @brief No clearing shoudl be done. */
    RENDERPASS_CLEAR_NONE_FLAG = 0x0,
    /** @brief Clear the color buffer. */
    RENDERPASS_CLEAR_COLOR_BUFFER_FLAG = 0x1,
    /** @brief Clear the depth buffer. */
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG = 0x2,
    /** @brief Clear the stencil buffer. */
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4
} E_RENDERPASS_CLEAR_FLAG;

typedef struct RENDERPASS_CONFIG {
    /** @brief The name of this renderpass. */
    const char* name;
    /** @brief The name of the previous renderpass. */
    const char* prev_name;
    /** @brief The name of the next renderpass. */
    const char* next_name;
    /** @brief The current render area of the renderpass. */
    Vector4 render_area;
    /** @brief The clear color used for this renderpass. */
    Vector4 clear_color;

    /** @brief The clear flags for this renderpass. */
    u8 clear_flags;
} RENDERPASS_CONFIG;

/**
 * @brief Represents a generic renderpass.
 */
typedef struct RENDERPASS {
    /** @brief The id of the renderpass */
    u16 id;

    /** @brief The current render area of the renderpass. */
    Vector4 render_area;
    /** @brief The clear color used for this renderpass. */
    Vector4 clear_color;

    /** @brief The clear flags for this renderpass. */
    u8 clear_flags;
    /** @brief The number of render targets for this renderpass. */
    u8 render_target_count;
    /** @brief An array of render targets used by this renderpass. */
    RENDER_TARGET* targets;

    /** @brief Internal renderpass data */
    void* internal_data;
} RENDERPASS;

typedef enum E_RENDERBUFFER_TYPE {
    /** @brief Buffer is use is unknown. Default, but usually invalid. */
    RENDERBUFFER_TYPE_UNKNOWN,
    /** @brief Buffer is used for vertex data. */
    RENDERBUFFER_TYPE_VERTEX,
    /** @brief Buffer is used for index data. */
    RENDERBUFFER_TYPE_INDEX,
    /** @brief Buffer is used for uniform data. */
    RENDERBUFFER_TYPE_UNIFORM,
    /** @brief Buffer is used for staging purposes (i.e. from host-visible to device-local memory) */
    RENDERBUFFER_TYPE_STAGING,
    /** @brief Buffer is used for reading purposes (i.e copy to from device local, then read) */
    RENDERBUFFER_TYPE_READ,
    /** @brief Buffer is used for data storage. */
    RENDERBUFFER_TYPE_STORAGE
} E_RENDERBUFFER_TYPE;

typedef struct RENDER_BUFFER {
    /** @brief The type of buffer, which typically determines its use. */
    E_RENDERBUFFER_TYPE type;
    /** @brief The total size of the buffer in bytes. */
    u64 total_size;
    /** @brief The amount of memory required to store the freelist. 0 if not used. */
    u64 freelist_memory_requirement;
    /** @brief The buffer freelist, if used. */
    FREELIST buffer_freelist;
    /** @brief The freelist memory block, if needed. */
    void* freelist_block;
    /** @brief Contains internal data for the renderer-API-specific buffer. */
    void* internal_data;
} RENDER_BUFFER;

/** @brief The generic configuration for a renderer backend. */
typedef struct RENDERER_BACKEND_CONFIG {
    /** @brief The name of the application */
    const char* application_name;
    /** @brief The number of pointers to renderpasses. */
    u16 renderpass_count;
    /** @brief An array configurations for renderpasses. Will be initialized on the backend automatically. */
    RENDERPASS_CONFIG* pass_configs;
    /** @brief A callback that will be made when the backend requires a refresh/regeneration of the render targets. */
    void (*on_rendertarget_refresh_required)(void);
} RENDERER_BACKEND_CONFIG;

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;
    E_RENDERER_BACKEND_API backend_api;
    u64 frame_number;

    b8 (*init)(struct RENDERER_BACKEND* backend, const RENDERER_BACKEND_CONFIG* config, u8* out_window_render_target_count);

    void (*shutdown)(struct RENDERER_BACKEND* backend);

    void (*resized)(struct RENDERER_BACKEND* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);
    b8 (*end_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);

    b8 (*renderpass_begin)(RENDERPASS* pass, RENDER_TARGET* target);
    b8 (*renderpass_end)(RENDERPASS* pass);
    RENDERPASS* (*renderpass_get)(const char* name);

    void (*draw_geometry)(GEOMETRY_RENDER_DATA* data);

    void (*texture_create)(const u8* pixels, struct TEXTURE* texture);
    void (*texture_destroy)(TEXTURE* texture);

    /**
     * @brief Creates a new writeable texture with no data written to it.
     *
     * @param t A pointer to the texture to hold the resources.
     */
    void (*texture_create_writeable)(TEXTURE* t);

    /**
     * @brief Resizes a texture. There is no check at this level to see if the
     * texture is writeable. Internal resources are destroyed and re-created at
     * the new resolution. Data is lost and would need to be reloaded.
     *
     * @param t A pointer to the texture to be resized.
     * @param new_width The new width in pixels.
     * @param new_height The new height in pixels.
     */
    void (*texture_resize)(TEXTURE* t, u32 new_width, u32 new_height);

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
    void (*texture_write_data)(TEXTURE* t, u32 offset, u32 size, const u8* pixels);

    b8 (*create_geometry)(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
    void (*destroy_geometry)(GEOMETRY* geometry);

    /**
     * @brief Creates internal shader resources using the provided parameters.
     * 
     * @param s A pointer to the shader.
     * @param config A constant pointer to the shader config.
     * @param pass A pointer to the renderpass to be associated with the shader.
     * @param stage_count The total number of stages.
     * @param stage_filenames An array of shader stage filenames to be loaded. Should align with stages array.
     * @param stages A array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
     * @return b8 True on success; otherwise false.
     */
    b8 (*shader_create)(struct SHADER* shader, const SHADER_CONFIG* config, RENDERPASS* pass, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);

    /**
     * @brief Destroys the given shader and releases any resources held by it.
     * @param s A pointer to the shader to be destroyed.
     */
    void (*shader_destroy)(struct SHADER* shader);

    /**
     * @brief Initializes a configured shader. Will be automatically destroyed if this step fails.
     * Must be done after vulkan_shader_create().
     *
     * @param s A pointer to the shader to be initialized.
     * @return True on success; otherwise false.
     */
    b8 (*shader_init)(struct SHADER* shader);

    /**
     * @brief Uses the given shader, activating it for updates to attributes, uniforms and such,
     * and for use in draw calls.
     *
     * @param s A pointer to the shader to be used.
     * @return True on success; otherwise false.
     */
    b8 (*shader_use)(struct SHADER* shader);

    /**
     * @brief Binds global resources for use and updating.
     *
     * @param s A pointer to the shader whose globals are to be bound.
     * @return True on success; otherwise false.
     */
    b8 (*shader_bind_globals)(struct SHADER* s);

    /**
     * @brief Binds instance resources for use and updating.
     *
     * @param s A pointer to the shader whose instance resources are to be bound.
     * @param instance_id The identifier of the instance to be bound.
     * @return True on success; otherwise false.
     */
    b8 (*shader_bind_instance)(struct SHADER* s, u32 instance_id);

    /**
     * @brief Applies global data to the uniform buffer.
     *
     * @param s A pointer to the shader to apply the global data for.
     * @return True on success; otherwise false.
     */
    b8 (*shader_apply_globals)(struct SHADER* s);

    /**
     * @brief Applies data for the currently bound instance.
     *
     * @param s A pointer to the shader to apply the instance data for.
     * @param needs_update Indicates if the shader uniforms need to be updated or just bound.
     * @return True on success; otherwise false.
     */
    b8 (*shader_apply_instance)(struct SHADER* s, b8 needs_update);

    /**
     * @brief Acquires internal instance-level resources and provides an instance id.
     *
     * @param s A pointer to the shader to acquire resources from.
     * @param maps An array of pointers to texture maps. Must be one map per instance texture.
     * @param out_instance_id A pointer to hold the new instance identifier.
     * @return True on success; otherwise false.
     */
    b8 (*shader_acquire_instance_resources)(struct SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id);

    /**
     * @brief Releases internal instance-level resources for the given instance id.
     *
     * @param s A pointer to the shader to release resources from.
     * @param instance_id The instance identifier whose resources are to be released.
     * @return True on success; otherwise false.
     */
    b8 (*shader_release_instance_resources)(struct SHADER* s, u32 instance_id);

    /**
     * @brief Sets the uniform of the given shader to the provided value.
     * 
     * @param s A ponter to the shader.
     * @param uniform A constant pointer to the uniform.
     * @param value A pointer to the value to be set.
     * @return b8 True on success; otherwise false.
     */
    b8 (*shader_set_uniform)(struct SHADER* frontend_shader, struct SHADER_UNIFORM* uniform, const void* value);

    b8 (*shader_after_renderpass)(struct SHADER* s);

    /**
     * @brief Acquires internal resources for the given texture map.
     * 
     * @param map A pointer to the texture map to obtain resources for.
     * @return True on success; otherwise false.
     */
    b8 (*texture_map_acquire_resources)(struct TEXTURE_MAP* map);

    /**
     * @brief Releases internal resources for the given texture map.
     * 
     * @param map A pointer to the texture map to release resources from.
     */
    void (*texture_map_release_resources)(struct TEXTURE_MAP* map);

    /**
     * @brief Creates a new render target using the provided data.
     *
     * @param attachment_count The number of attachments (texture pointers).
     * @param attachments An array of attachments (texture pointers).
     * @param renderpass A pointer to the renderpass the render target is associated with.
     * @param width The width of the render target in pixels.
     * @param height The height of the render target in pixels.
     * @param out_target A pointer to hold the newly created render target.
     */
    void (*render_target_create)(u8 attachment_count, TEXTURE** attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target);

    /**
     * @brief Destroys the provided render target.
     *
     * @param target A pointer to the render target to be destroyed.
     * @param free_internal_memory Indicates if internal memory should be freed.
     */
    void (*render_target_destroy)(RENDER_TARGET* target, b8 free_internal_memory);

    /**
     * @brief Creates a new renderpass.
     *
     * @param out_renderpass A pointer to the generic renderpass.
     * @param depth The depth clear amount.
     * @param stencil The stencil clear value.
     * @param clear_flags The combined clear flags indicating what kind of clear should take place.
     * @param has_prev_pass Indicates if there is a previous renderpass.
     * @param has_next_pass Indicates if there is a next renderpass.
     */
    void (*renderpass_create)(RENDERPASS* out_renderpass, f32 depth, u32 stencil, b8 has_prev_pass, b8 has_next_pass);

    /**
     * @brief Destroys the given renderpass.
     *
     * @param pass A pointer to the renderpass to be destroyed.
     */
    void (*renderpass_destroy)(RENDERPASS* pass);

    /**
     * @brief Attempts to get the window render target at the given index.
     *
     * @param index The index of the attachment to get. Must be within the range of window render target count.
     * @return A pointer to a texture attachment if successful; otherwise 0.
     */
    TEXTURE* (*window_attachment_get)(u8 index);

    /**
     * @brief Returns a pointer to the main depth texture target.
     */
    TEXTURE* (*depth_attachment_get)(void);

    /**
     * @brief Returns the current window attachment index.
     */
    u8 (*window_attachment_index_get)(void);

    /**
     * @brief Indicates if the renderer is capable of multi-threading.
     */
    b8 (*is_multithreaded)(void);

    /**
     * @brief Creates and assigns the renderer-backend-specific buffer.
     *
     * @param buffer A pointer to create the internal buffer for.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_create_internal)(RENDER_BUFFER* buffer);

    /**
     * @brief Destroys the given buffer.
     *
     * @param buffer A pointer to the buffer to be destroyed.
     */
    void (*renderbuffer_destroy_internal)(RENDER_BUFFER* buffer);

    /**
     * @brief Binds the given buffer at the provided offset.
     *
     * @param buffer A pointer to the buffer to bind.
     * @param offset The offset in bytes from the beginning of the buffer.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_bind)(RENDER_BUFFER* buffer, u64 offset);
    /**
     * @brief Unbinds the given buffer.
     *
     * @param buffer A pointer to the buffer to be unbound.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_unbind)(RENDER_BUFFER* buffer);

    /**
     * @brief Maps memory from the given buffer in the provided range to a block of memory and returns it.
     * This memory should be considered invalid once unmapped.
     * @param buffer A pointer to the buffer to map.
     * @param offset The number of bytes from the beginning of the buffer to map.
     * @param size The amount of memory in the buffer to map.
     * @returns A mapped block of memory. Freed and invalid once unmapped.
     */
    void* (*renderbuffer_map_memory)(RENDER_BUFFER* buffer, u64 offset, u64 size);
    /**
     * @brief Unmaps memory from the given buffer in the provided range to a block of memory.
     * This memory should be considered invalid once unmapped.
     * @param buffer A pointer to the buffer to unmap.
     * @param offset The number of bytes from the beginning of the buffer to unmap.
     * @param size The amount of memory in the buffer to unmap.
     */
    void (*renderbuffer_unmap_memory)(RENDER_BUFFER* buffer, u64 offset, u64 size);

    /**
     * @brief Flushes buffer memory at the given range. Should be done after a write.
     * @param buffer A pointer to the buffer to unmap.
     * @param offset The number of bytes from the beginning of the buffer to flush.
     * @param size The amount of memory in the buffer to flush.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_flush)(RENDER_BUFFER* buffer, u64 offset, u64 size);

    /**
     * @brief Reads memory from the provided buffer at the given range to the output variable.
     * @param buffer A pointer to the buffer to read from.
     * @param offset The number of bytes from the beginning of the buffer to read.
     * @param size The amount of memory in the buffer to read.
     * @param out_memory A pointer to a block of memory to read to. Must be of appropriate size.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_read)(RENDER_BUFFER* buffer, u64 offset, u64 size, void** out_memory);

    /**
     * @brief Resizes the given buffer to new_total_size. new_total_size must be
     * greater than the current buffer size. Data from the old internal buffer is copied
     * over.
     *
     * @param buffer A pointer to the buffer to be resized.
     * @param new_total_size The new size in bytes. Must be larger than the current size.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_resize)(RENDER_BUFFER* buffer, u64 new_total_size);

    /**
     * @brief Loads provided data into the specified rage of the given buffer.
     *
     * @param buffer A pointer to the buffer to load data into.
     * @param offset The offset in bytes from the beginning of the buffer.
     * @param size The size of the data in bytes to be loaded.
     * @param data The data to be loaded.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_load_range)(RENDER_BUFFER* buffer, u64 offset, u64 size, const void* data);

    /**
     * @brief Copies data in the specified rage fron the source to the destination buffer.
     *
     * @param source A pointer to the source buffer to copy data from.
     * @param source_offset The offset in bytes from the beginning of the source buffer.
     * @param dest A pointer to the destination buffer to copy data to.
     * @param dest_offset The offset in bytes from the beginning of the destination buffer.
     * @param size The size of the data in bytes to be copied.
     * @returns True on success; otherwise false.
     */
    b8 (*renderbuffer_copy_range)(RENDER_BUFFER* source, u64 source_offset, RENDER_BUFFER* dest, u64 dest_offset, u64 size);

    /**
     * @brief Attempts to draw the contents of the provided buffer at the given offset
     * and element count. Only meant for use with vertex and index buffers.
     *
     * @param buffer A pointer to the buffer to be drawn.
     * @param offset The offset in bytes from the beginning of the buffer.
     * @param element_count The number of elements to be drawn.
     * @param bind_only Only binds the buffer, but does not call draw.
     * @return True on success; otherwise false.
     */
    b8 (*renderbuffer_draw)(RENDER_BUFFER* buffer, u64 offset, u32 element_count, b8 bind_only);
} RENDERER_BACKEND;

/** @brief Known render view types, which have logic associated with them. */
typedef enum E_RENDER_VIEW_KNOWN_TYPE {
    /** @brief A view which only renders objects with *no* transparency. */
    RENDERER_VIEW_KNOWN_TYPE_WORLD = 0x01,
    /** @brief A view which only renders ui objects. */
    RENDERER_VIEW_KNOWN_TYPE_UI = 0x02,
    /** @brief A view which only renders skybox objects. */
    RENDERER_VIEW_KNOWN_TYPE_SKYBOX = 0x03,
} E_RENDER_VIEW_KNOWN_TYPE;

/** @brief Known view matrix sources. */
typedef enum E_RENDER_VIEW_VIEW_MATRIX_SOURCE {
    RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA = 0x01,
    RENDER_VIEW_VIEW_MATRIX_SOURCE_UI_CAMERA = 0x02,
    RENDER_VIEW_VIEW_MATRIX_SOURCE_LIGHT_CAMERA = 0x03,
} E_RENDER_VIEW_VIEW_MATRIX_SOURCE;

/** @brief Known projection matrix sources. */
typedef enum E_RENDER_VIEW_PROJECTION_MATRIX_SOURCE {
    RENDER_VIEW_PROJECTION_MATRIX_SOURCE_DEFAULT_PERSPECTIVE = 0x01,
    RENDER_VIEW_PROJECTION_MATRIX_SOURCE_DEFAULT_ORTHOGRAPHIC = 0x02,
} E_RENDER_VIEW_PROJECTION_MATRIX_SOURCE;

/** @brief configuration for a renderpass to be associated with a view */
typedef struct RENDER_VIEW_PASS_CONFIG {
    const char* name;
} RENDER_VIEW_PASS_CONFIG;

/**
 * @brief The configuration of a render view.
 * Used as a serialization target.
 */
typedef struct RENDER_VIEW_CONFIG {
    /** @brief The name of the view. */
    const char* name;

    /**
     * @brief The name of a custom shader to be used
     * instead of the view's default. Must be 0 if
     * not used.
     */
    const char* custom_shader_name;
    /** @brief The width of the view. Set to 0 for 100% width. */
    u16 width;
    /** @brief The height of the view. Set to 0 for 100% height. */
    u16 height;
    /** @brief The known type of the view. Used to associate with view logic. */
    E_RENDER_VIEW_KNOWN_TYPE type;
    /** @brief The source of the view matrix. */
    E_RENDER_VIEW_VIEW_MATRIX_SOURCE view_matrix_source;
    /** @brief The source of the projection matrix. */
    E_RENDER_VIEW_PROJECTION_MATRIX_SOURCE projection_matrix_source;
    /** @brief The number of renderpasses used in this view. */
    u8 pass_count;
    /** @brief The configuration of renderpasses used in this view. */
    RENDER_VIEW_PASS_CONFIG* passes;
} RENDER_VIEW_CONFIG;

struct RENDER_VIEW_PACKET;

/**
 * @brief A render view instance, responsible for the generation
 * of view packets based on internal logic and given config.
 */
typedef struct RENDER_VIEW {
    /** @brief The unique identifier of this view. */
    u16 id;
    /** @brief The name of the view. */
    const char* name;
    /** @brief The current width of this view. */
    u16 width;
    /** @brief The current height of this view. */
    u16 height;
    /** @brief The known type of this view. */
    E_RENDER_VIEW_KNOWN_TYPE type;

    /** @brief The number of renderpasses used by this view. */
    u8 renderpass_count;
    /** @brief An array of pointers to renderpasses used by this view. */
    RENDERPASS** passes;

    /** @brief The name of the custom shader used by this view, if there is one. */
    const char* custom_shader_name;
    /** @brief The internal, view-specific data for this view. */
    void* internal_data;

    /**
     * @brief A pointer to a function to be called when this view is created.
     *
     * @param self A pointer to the view being created.
     * @return True on success; otherwise false.
     */
    b8 (*on_create)(struct RENDER_VIEW* self);
    /**
     * @brief A pointer to a function to be called when this view is destroyed.
     *
     * @param self A pointer to the view being destroyed.
     */
    void (*on_destroy)(struct RENDER_VIEW* self);

    /**
     * @brief A pointer to a function to be called when the owner of this view (such
     * as the window) is resized.
     *
     * @param self A pointer to the view being resized.
     * @param width The new width in pixels.
     * @param width The new height in pixels.
     */
    void (*on_resize)(struct RENDER_VIEW* self, u32 width, u32 height);

    /**
     * @brief Builds a render view packet using the provided view and meshes.
     *
     * @param self A pointer to the view to use.
     * @param data Freeform data used to build the packet.
     * @param out_packet A pointer to hold the generated packet.
     * @return True on success; otherwise false.
     */
    b8 (*on_build_packet)(const struct RENDER_VIEW* self, void* data, struct RENDER_VIEW_PACKET* out_packet);

    /**
     * @brief Destroys the provided render view packet.
     * @param self A pointer to the view to use.
     * @param packet A pointer to the packet to be destroyed.
     */
    void (*on_destroy_packet)(const struct RENDER_VIEW* self, struct RENDER_VIEW_PACKET* packet);

    /**
     * @brief Uses the given view and packet to render the contents therein.
     *
     * @param self A pointer to the view to use.
     * @param packet A pointer to the packet whose data is to be rendered.
     * @param frame_number The current renderer frame number, typically used for data synchronization.
     * @param render_target_index The current render target index for renderers that use multiple render targets at once (i.e. Vulkan).
     * @return True on success; otherwise false.
     */
    b8 (*on_render)(const struct RENDER_VIEW* self, const struct RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index);
} RENDER_VIEW;

/**
 * @brief A packet for and generated by a render view, which contains
 * data about what is to be rendered.
 */
typedef struct RENDER_VIEW_PACKET {
    /** @brief A constant pointer to the view this packet is associated with. */
    const RENDER_VIEW* view;
    /** @brief The current view matrix. */
    Matrice4 view_matrix;
    /** @brief The current projection matrix. */
    Matrice4 projection_matrix;
    /** @brief The current view position, if applicable. */
    Vector3 view_position;
    /** @brief The current scene ambient color, if applicable. */
    Vector4 ambient_color;
    /** @brief The number of geometries to be drawn. */
    u32 geometry_count;
    /** @brief The geometries to be drawn. */
    GEOMETRY_RENDER_DATA* geometries;
    /** @brief The name of the custom shader to use, if applicable. Otherwise 0. */
    const char* custom_shader_name;
    /** @brief Holds a pointer to freeform data, typically understood both by the object and consuming view. */
    void* extended_data;
} RENDER_VIEW_PACKET;

typedef struct MESH_PACKET_DATA {
    u32 mesh_count;
    Mesh** meshes;
} MESH_PACKET_DATA;

struct UI_TEXT;
typedef struct UI_PACKET_DATA {
    MESH_PACKET_DATA mesh_data;
    // TODO: temp
    u32 text_count;
    struct UI_TEXT** texts;
} UI_PACKET_DATA;

typedef struct SKYBOX_PACKET_DATA {
    Skybox* sb;
} SKYBOX_PACKET_DATA;


/*
 * @brief A structure which is generated by the application and sent once
 * to the renderer to render a given frame. Consists of any data required,
 * such as delta time and a collection of views to be rendered.
 */
typedef struct RENDER_PACKET {
    f32 delta_time;

    /** The number of views to be rendered. */
    u16 view_count;
    /** An array of views to be rendered. */
    RENDER_VIEW_PACKET* views;
} RENDER_PACKET;
