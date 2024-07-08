#include "vulkan_backend.h"

#include "vulkan_types.inl"

#include "core/logger.h"

// static Vulkan context
static VULKAN_CONTEXT context;

b8 vulkan_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name, struct PLATFORM_STATE* platform_state) {
    // TODO: custom allocator.
    context.allocator = 0;

    // Setup Vulkan instance.
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Ywmaa Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = 0;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = 0;

    VkResult result = vkCreateInstance(&create_info, context.allocator, &context.instance);
    if(result != VK_SUCCESS) {
        PRINT_ERROR("vkCreateInstance failed with result: %u", result);
        return FALSE;
    }

    PRINT_INFO("Vulkan renderer initialized successfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
}

void vulkan_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height) {
}

b8 vulkan_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    return TRUE;
}