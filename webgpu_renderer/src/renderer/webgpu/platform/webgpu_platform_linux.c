#include <platform/platform.h>

// Linux platform layer.
#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

#include "../thirdparty/x11_loader/X11_loader.h"

#include <data_structures/darray.h>
#include <core/ymemory.h>
#include <core/logger.h>

#include "renderer/webgpu/webgpu_types.inl"

typedef struct X11_HANDLE_INFO {
    xcb_connection_t* connection;
    xcb_window_t window;
} X11_HANDLE_INFO;

// X11-specific WebGPU surface creation
b8 platform_x11_create_webgpu_surface(WEBGPU_CONTEXT* context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    X11_HANDLE_INFO* handle = (X11_HANDLE_INFO*)block;
    
    // Create WebGPU surface descriptor
    WGPUSurfaceSourceXCBWindow xcbDesc = {0};
    xcbDesc.chain.sType = WGPUSType_SurfaceSourceXCBWindow;
    xcbDesc.chain.next = NULL;
    xcbDesc.connection = handle->connection;
    xcbDesc.window = handle->window;
    
    WGPUSurfaceDescriptor descriptor = {0};
    descriptor.nextInChain = (const WGPUChainedStruct*)&xcbDesc;
    //descriptor.label = NULL;
    
    context->surface = wgpuInstanceCreateSurface(context->instance, &descriptor);

    return true;
}
#ifdef WAYLAND_ENABLED

typedef struct WAYLAND_HANDLE_INFO {
    struct wl_display* display;
    struct wl_surface* surface;
} WAYLAND_HANDLE_INFO;

// Wayland-specific WebGPU surface creation
b8 platform_wayland_create_webgpu_surface(WEBGPU_CONTEXT* context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    WAYLAND_HANDLE_INFO* handle = (WAYLAND_HANDLE_INFO*)block;
    
    WGPUSurfaceSourceWaylandSurface waylandDesc = {0};
    waylandDesc.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
    waylandDesc.chain.next = NULL;
    waylandDesc.display = handle->display;
    waylandDesc.surface = handle->surface;
    
    WGPUSurfaceDescriptor descriptor = {0};
    descriptor.nextInChain = (const WGPUChainedStruct*)&waylandDesc;
    //descriptor.label = NULL;
    
    context->surface = wgpuInstanceCreateSurface(context->instance, &descriptor);

    return true;
}
#endif
// WebGPU surface creation based on active backend
b8 platform_create_webgpu_surface(struct WEBGPU_CONTEXT* context) {
    if (!context) {
        return false;
    }
    
    switch (platform_get_linux_active_display_protocol()) {
        case PLATFORM_BACKEND_WAYLAND:
            #ifdef WAYLAND_ENABLED
            return platform_wayland_create_webgpu_surface(context);
            #endif
            break;
            
        case PLATFORM_BACKEND_X11:
        default:
            return platform_x11_create_webgpu_surface(context);
            break;
    }
    
    return false;
}
#endif // defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)
