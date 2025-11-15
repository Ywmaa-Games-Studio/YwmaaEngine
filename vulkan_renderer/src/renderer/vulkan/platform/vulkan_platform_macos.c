#include "platform/platform.h"

#if defined(YPLATFORM_APPLE)

#include <data_structures/darray.h>
#include <platform/platform.h>
#include <core/ymemory.h>
#include <core/logger.h>

// For surface creation
#define VK_USE_PLATFORM_METAL_EXT
#define VOLK_IMPLEMENTATION
#include "../thirdparty/volk/volk.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/platform/vulkan_platform.h"

typedef struct MACOS_HANDLE_INFO {
    CAMetalLayer* layer;
} MACOS_HANDLE_INFO;


void platform_get_required_extension_names(const char ***names_darray) {
    darray_push(*names_darray, &"VK_EXT_metal_surface");
    // Required for macos
    darray_push(*names_darray, &VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
}

b8 platform_create_vulkan_surface(VULKAN_CONTEXT *context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);


    VkMetalSurfaceCreateInfoEXT create_info = {VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
    create_info.pLayer = ((MACOS_HANDLE_INFO*)block)->layer;

    VkResult result = vkCreateMetalSurfaceEXT(
        context->instance, 
        &create_info,
        context->allocator,
        &context->surface);
    if (result != VK_SUCCESS) {
        PRINT_ERROR("Vulkan surface creation failed.");
        return false;
    }

    return true;
}

#endif
