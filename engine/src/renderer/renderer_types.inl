#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"
typedef enum E_RENDERER_BACKEND_API {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_WEBGPU,
} E_RENDERER_BACKEND_API;

typedef struct GLOBAL_UNIFORM_OBJECT {
    Matrice4 projection;   // 64 bytes
    Matrice4 view;         // 64 bytes
    Matrice4 m_reserved0;  // 64 bytes, reserved for future use
    Matrice4 m_reserved1;  // 64 bytes, reserved for future use
} GLOBAL_UNIFORM_OBJECT;

typedef struct MATERIAL_UNIFORM_OBJECT {
    Vector4 diffuse_color;  // 16 bytes
    Vector4 v_reserved0;    // 16 bytes, reserved for future use
    Vector4 v_reserved1;    // 16 bytes, reserved for future use
    Vector4 v_reserved2;    // 16 bytes, reserved for future use
} MATERIAL_UNIFORM_OBJECT;

typedef struct GEOMETRY_RENDER_DATA {
    u32 object_id;
    Matrice4 model;
    MATERIAL* material;
} GEOMETRY_RENDER_DATA;

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;
    u64 frame_number;

    b8 (*init)(struct RENDERER_BACKEND* backend, const char* application_name);

    void (*shutdown)(struct RENDERER_BACKEND* backend);

    void (*resized)(struct RENDERER_BACKEND* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);
    void (*update_global_state)(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode);
    b8 (*end_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);

    void (*update_object)(GEOMETRY_RENDER_DATA data);

    void (*create_texture)(const u8* pixels, struct TEXTURE* texture);
    void (*destroy_texture)(TEXTURE* texture);

    b8 (*create_material)(struct MATERIAL* material);
    void (*destroy_material)(struct MATERIAL* material);
} RENDERER_BACKEND;

typedef struct RENDER_PACKET {
    f32 delta_time;
} RENDER_PACKET;