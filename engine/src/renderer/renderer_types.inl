#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"
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

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;
    u64 frame_number;

    b8 (*init)(struct RENDERER_BACKEND* backend, const char* application_name);

    void (*shutdown)(struct RENDERER_BACKEND* backend);

    void (*resized)(struct RENDERER_BACKEND* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);
    void (*update_global_world_state)(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode);
    void (*update_global_ui_state)(Matrice4 projection, Matrice4 view, i32 mode);
    b8 (*end_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);

    b8 (*begin_renderpass)(struct RENDERER_BACKEND* backend, u8 renderpass_id);
    b8 (*end_renderpass)(struct RENDERER_BACKEND* backend, u8 renderpass_id);

    void (*draw_geometry)(GEOMETRY_RENDER_DATA data);

    void (*create_texture)(const u8* pixels, struct TEXTURE* texture);
    void (*destroy_texture)(TEXTURE* texture);

    b8 (*create_material)(struct MATERIAL* material);
    void (*destroy_material)(struct MATERIAL* material);

    b8 (*create_geometry)(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
    void (*destroy_geometry)(GEOMETRY* geometry);
} RENDERER_BACKEND;

typedef struct RENDER_PACKET {
    f32 delta_time;
    u32 geometry_count;
    GEOMETRY_RENDER_DATA* geometries;

    u32 ui_geometry_count;
    GEOMETRY_RENDER_DATA* ui_geometries;
} RENDER_PACKET;
