#pragma once

#include "renderer/renderer_backend.h"

b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name, struct PLATFORM_STATE* platform_state);
void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend);

void webgpu_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height);

b8 webgpu_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time);
b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time);