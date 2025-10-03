#include "webgpu_swapchain.h"
#include "core/logger.h"

b8 webgpu_swapchain_create(WEBGPU_CONTEXT* context,
    u32 width,
    u32 height) {
    WGPUSurfaceConfiguration config = {0};
    config.nextInChain = NULL;
    config.width = width;
    config.height = height;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.format = context->swapchain_format;
    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = NULL;
    config.device = context->device;
    config.presentMode = context->flags & RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(context->surface, &config);
    

    return context->surface != NULL;
}

void webgpu_swapchain_destroy(WEBGPU_CONTEXT* context){
    wgpuSurfaceUnconfigure(context->surface);
}

b8 webgpu_recreate_swapchain(WEBGPU_CONTEXT* context, RENDERER_BACKEND* backend, u32 width, u32 height) {
    webgpu_swapchain_destroy(context);
    
    // Re-init
    // Swapchain
    if (!webgpu_swapchain_create(context, width, height)){
        PRINT_DEBUG("Failed to recreate swapchain");
        return false;
    }

    return true;
}
