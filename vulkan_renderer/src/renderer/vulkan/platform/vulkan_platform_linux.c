#include <platform/platform.h>

// Linux platform layer.
#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

#include "../thirdparty/x11_loader/X11_loader.h"

#include <data_structures/darray.h>
#include <core/ymemory.h>
#include <core/logger.h>

// Include platform-specific Vulkan surface extensions
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR

// Include Volk
#define VOLK_IMPLEMENTATION
#include "../thirdparty/volk/volk.h"

// For Vulkan surface creation
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/platform/vulkan_platform.h"

typedef struct X11_HANDLE_INFO {
    xcb_connection_t* connection;
    xcb_window_t window;
} X11_HANDLE_INFO;

// Platform-specific function that returns required extension names based on the active backend
void platform_get_required_extension_names(const char*** names_darray) {
    // Return appropriate extensions based on active backend
#ifdef WAYLAND_ENABLED
    darray_push(*names_darray, &"VK_KHR_wayland_surface");
#endif
    darray_push(*names_darray, &"VK_KHR_xcb_surface");  // VK_KHR_xlib_surface?
}

// X11-specific Vulkan surface creation
b8 platform_x11_create_vulkan_surface(VULKAN_CONTEXT* context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    X11_HANDLE_INFO* handle = (X11_HANDLE_INFO*)block;
    
    xcb_connection_t* connection = handle->connection;
    xcb_window_t window = handle->window;
    
    if (!connection) {
        PRINT_ERROR("Invalid X11 connection for Vulkan surface creation");
        return false;
    }
    
    VkXcbSurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    create_info.connection = connection;
    create_info.window = window;
    
    VkResult result = vkCreateXcbSurfaceKHR(
        context->instance,
        &create_info,
        context->allocator,
        &context->surface);
    
    if (result != VK_SUCCESS) {
        PRINT_ERROR("Vulkan surface creation failed for X11.");
        return false;
    }

    return true;
}
#ifdef WAYLAND_ENABLED
typedef struct WAYLAND_HANDLE_INFO {
    struct wl_display* display;
    struct wl_surface* surface;
} WAYLAND_HANDLE_INFO;

// Wayland-specific Vulkan surface creation
b8 platform_wayland_create_vulkan_surface(VULKAN_CONTEXT* context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    WAYLAND_HANDLE_INFO* handle = (WAYLAND_HANDLE_INFO*)block;
    
    VkWaylandSurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
    create_info.display = handle->display;
    create_info.surface = handle->surface;
    
    VkResult result = vkCreateWaylandSurfaceKHR(
        context->instance,
        &create_info,
        context->allocator,
        &context->surface);
    
    if (result != VK_SUCCESS) {
        PRINT_ERROR("Vulkan Wayland surface creation failed.");
        return false;
    }

    return true;
}
#endif
// Vulkan surface creation based on active backend
b8 platform_create_vulkan_surface(VULKAN_CONTEXT* context) {
    if (!context) {
        return false;
    }
    
    switch (platform_get_linux_active_display_protocol()) {
        case PLATFORM_BACKEND_WAYLAND:
            #ifdef WAYLAND_ENABLED
            return platform_wayland_create_vulkan_surface(context);
            #endif
            break;
            
        case PLATFORM_BACKEND_X11:
        default:
            return platform_x11_create_vulkan_surface(context);
            break;
    }
    
    return false;
}
#endif // defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)
