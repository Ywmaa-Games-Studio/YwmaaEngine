#include <platform/platform.h>
// Windows Platform Layer.
#if YPLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <data_structures/darray.h>
#include <platform/platform.h>
#include <core/ymemory.h>
#include <core/logger.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "../thirdparty/volk/volk.h"
#include <vulkan/vulkan_win32.h>
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/platform/vulkan_platform.h"

typedef struct WIN32_HANDLE_INFO {
    HINSTANCE h_instance;
    HWND hwnd;
} WIN32_HANDLE_INFO;
    
void platform_get_required_extension_names(const char ***names_darray) {
    darray_push(*names_darray, &"VK_KHR_win32_surface");
}

// Surface creation for Vulkan
b8 platform_create_vulkan_surface(VULKAN_CONTEXT *context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    WIN32_HANDLE_INFO *handle = (WIN32_HANDLE_INFO *)block;

    if (!handle) {
        return false;
    }

    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hinstance = handle->h_instance;
    create_info.hwnd = handle->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &context->surface);
    if (result != VK_SUCCESS) {
        PRINT_ERROR("Vulkan surface creation failed.");
        return false;
    }

    return true;
}
#endif
