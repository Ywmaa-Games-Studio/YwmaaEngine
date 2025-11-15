#include "webgpu_types.inl"

b8 webgpu_swapchain_create(WEBGPU_CONTEXT* context,
    u32 width,
    u32 height);
void webgpu_swapchain_destroy(WEBGPU_CONTEXT* context);
b8 webgpu_recreate_swapchain(WEBGPU_CONTEXT* context, RENDERER_PLUGIN* backend, u32 cached_framebuffer_width, u32 cached_framebuffer_height);
