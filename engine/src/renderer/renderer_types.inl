#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

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

typedef enum E_BUILTIN_RENDERPASS {
    BUILTIN_RENDERPASS_WORLD = 0x01,
    BUILTIN_RENDERPASS_UI = 0x02,
} E_BUILTIN_RENDERPASS;

typedef enum E_RENDERER_DEBUG_VIEW_MODE {
    RENDERER_VIEW_MODE_DEFAULT = 0,
    RENDERER_VIEW_MODE_LIGHTING = 1,
    RENDERER_VIEW_MODE_NORMALS = 2
} E_RENDERER_DEBUG_VIEW_MODE;

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;
    E_RENDERER_BACKEND_API backend_api;
    u64 frame_number;

    b8 (*init)(struct RENDERER_BACKEND* backend, const char* application_name);

    void (*shutdown)(struct RENDERER_BACKEND* backend);

    void (*resized)(struct RENDERER_BACKEND* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);
    b8 (*end_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);

    b8 (*begin_renderpass)(struct RENDERER_BACKEND* backend, u8 renderpass_id);
    b8 (*end_renderpass)(struct RENDERER_BACKEND* backend, u8 renderpass_id);

    void (*draw_geometry)(GEOMETRY_RENDER_DATA data);

    void (*create_texture)(const u8* pixels, struct TEXTURE* texture);
    void (*destroy_texture)(TEXTURE* texture);

    b8 (*create_geometry)(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
    void (*destroy_geometry)(GEOMETRY* geometry);

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
    b8 (*shader_create)(struct SHADER* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages);

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
     * @param out_instance_id A pointer to hold the new instance identifier.
     * @return True on success; otherwise false.
     */
    b8 (*shader_acquire_instance_resources)(struct SHADER* s, u32* out_instance_id);

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
} RENDERER_BACKEND;

typedef struct RENDER_PACKET {
    f32 delta_time;
    u32 geometry_count;
    GEOMETRY_RENDER_DATA* geometries;

    u32 ui_geometry_count;
    GEOMETRY_RENDER_DATA* ui_geometries;
} RENDER_PACKET;
