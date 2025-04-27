#pragma once

#include "renderer_types.inl"

struct PLATFORM_STATE;

b8 renderer_backend_create(E_RENDERER_BACKEND_API type, RENDERER_BACKEND* out_renderer_backend);
void renderer_backend_destroy(RENDERER_BACKEND* renderer_backend);
