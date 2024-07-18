#include "renderer_backend.h"

#include "vulkan/vulkan_backend.h"
#include "webgpu/webgpu_backend.h"

b8 renderer_backend_create(E_RENDERER_BACKEND_API type, struct PLATFORM_STATE* platform_state, RENDERER_BACKEND* out_renderer_backend) {
    out_renderer_backend->platform_state = platform_state;

    if (type == RENDERER_BACKEND_API_VULKAN) {
        out_renderer_backend->init = vulkan_renderer_backend_init;
        out_renderer_backend->shutdown = vulkan_renderer_backend_shutdown;
        out_renderer_backend->begin_frame = vulkan_renderer_backend_begin_frame;
        out_renderer_backend->end_frame = vulkan_renderer_backend_end_frame;
        out_renderer_backend->resized = vulkan_renderer_backend_on_resized;

        return TRUE;
    }

    if (type == RENDERER_BACKEND_API_WEBGPU) {
        out_renderer_backend->init = webgpu_renderer_backend_init;
        out_renderer_backend->shutdown = webgpu_renderer_backend_shutdown;
        out_renderer_backend->begin_frame = webgpu_renderer_backend_begin_frame;
        out_renderer_backend->end_frame = webgpu_renderer_backend_end_frame;
        out_renderer_backend->resized = webgpu_renderer_backend_on_resized;

        return TRUE;
    }

    return FALSE;
}

void renderer_backend_destroy(RENDERER_BACKEND* renderer_backend) {
    renderer_backend->init = 0;
    renderer_backend->shutdown = 0;
    renderer_backend->begin_frame = 0;
    renderer_backend->end_frame = 0;
    renderer_backend->resized = 0;
}