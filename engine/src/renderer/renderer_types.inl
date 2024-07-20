#pragma once

#include "defines.h"
#include "math/math_types.h"
typedef enum E_RENDERER_BACKEND_API {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_WEBGPU,
} E_RENDERER_BACKEND_API;

typedef struct global_uniform_object {
    Matrice4 projection;   // 64 bytes
    Matrice4 view;         // 64 bytes
    Matrice4 m_reserved0;  // 64 bytes, reserved for future use
    Matrice4 m_reserved1;  // 64 bytes, reserved for future use
} global_uniform_object;

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;
    u64 frame_number;

    b8 (*init)(struct RENDERER_BACKEND* backend, const char* application_name);

    void (*shutdown)(struct RENDERER_BACKEND* backend);

    void (*resized)(struct RENDERER_BACKEND* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);
    void (*update_global_state)(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode);
    b8 (*end_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);    
} RENDERER_BACKEND;

typedef struct RENDER_PACKET {
    f32 delta_time;
} RENDER_PACKET;