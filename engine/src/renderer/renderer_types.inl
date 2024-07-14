#pragma once

#include "defines.h"

typedef enum E_RENDERER_BACKEND_API {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_OPENGL,
    RENDERER_BACKEND_API_DIRECTX
} E_RENDERER_BACKEND_API;

typedef struct RENDERER_BACKEND {
    struct PLATFORM_STATE* platform_state;
    u64 frame_number;

    b8 (*init)(struct RENDERER_BACKEND* backend, const char* application_name, struct PLATFORM_STATE* platform_state);

    void (*shutdown)(struct RENDERER_BACKEND* backend);

    void (*resized)(struct RENDERER_BACKEND* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);
    b8 (*end_frame)(struct RENDERER_BACKEND* backend, f32 delta_time);    
} RENDERER_BACKEND;

typedef struct RENDER_PACKET {
    f32 delta_time;
} RENDER_PACKET;