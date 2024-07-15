#include "vulkan_backend.h"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_types.inl"

#include "core/logger.h"
#include "core/ystring.h"

#include "variants/darray.h"

#include "platform/platform.h"

// Since these are just for clean code, I put/declare them in .c file
void vulkan_setup_extensions(VkInstanceCreateInfo* create_info);
b8 vulkan_setup_validation_layers(VkInstanceCreateInfo* create_info);
void vulkan_setup_debugger();

// static Vulkan context
static VULKAN_CONTEXT context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

b8 vulkan_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name, struct PLATFORM_STATE* platform_state) {
    VK_CHECK(volkInitialize());
    // TODO: custom allocator.
    context.allocator = 0;

    //START Setup Vulkan instance.
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Ywmaa Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    

    vulkan_setup_extensions(&create_info);

    if (vulkan_setup_validation_layers(&create_info) == FALSE) return FALSE;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    volkLoadInstance(context.instance);
    PRINT_INFO("Vulkan Instance created.");
    //END Setup Vulkan instance.

    vulkan_setup_debugger();

    //START Surface
    PRINT_DEBUG("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(platform_state, &context)) {
        PRINT_ERROR("Failed to create platform surface!");
        return FALSE;
    }
    PRINT_DEBUG("Vulkan surface created.");
    //END

    //START Device creation
    if (!vulkan_device_create(&context)) {
        PRINT_ERROR("Failed to create device!");
        return FALSE;
    }
    //END

    PRINT_INFO("Vulkan renderer initialized successfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
#if defined(_DEBUG)
    PRINT_DEBUG("Destroying Vulkan debugger...");
    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
    }
#endif

    PRINT_DEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height) {
}

b8 vulkan_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    return TRUE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PRINT_ERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PRINT_WARNING(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PRINT_INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PRINT_INFO(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}



void vulkan_setup_extensions(VkInstanceCreateInfo* create_info){
    //START Obtain a list of required extensions
    const char** required_extensions = darray_create(const char*);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);  // Generic surface extension
    platform_get_required_extension_names(&required_extensions);       // Platform-specific extension(s)
#if defined(_DEBUG)
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // debug utilities

    PRINT_DEBUG("Required extensions:");
    u32 length = darray_length(required_extensions);
    for (u32 i = 0; i < length; ++i) {
        PRINT_DEBUG(required_extensions[i]);
    }
#endif

    create_info->enabledExtensionCount = darray_length(required_extensions);
    create_info->ppEnabledExtensionNames = required_extensions;
    //END Obtain a list of required extensions
}


b8 vulkan_setup_validation_layers(VkInstanceCreateInfo* create_info){
    //START Validation layers.
    const char** required_validation_layer_names = 0;
    u32 required_validation_layer_count = 0;

// If validation should be done, get a list of the required validation layert names
// and make sure they exist. Validation layers should only be enabled on non-release builds.
#if defined(_DEBUG)
    PRINT_INFO("Validation layers enabled. Enumerating...");

    // The list of validation layers required.
    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = darray_length(required_validation_layer_names);

    // Obtain a list of available validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    // Verify all required layers are available.
    for (u32 i = 0; i < required_validation_layer_count; ++i) {
        PRINT_INFO("Searching for layer: %s...", required_validation_layer_names[i]);
        b8 found = FALSE;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (strings_equal(required_validation_layer_names[i], available_layers[j].layerName)) {
                found = TRUE;
                PRINT_INFO("Found.");
                break;
            }
        }

        if (!found) {
            PRINT_ERROR("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return FALSE;
        }
    }
    PRINT_INFO("All required validation layers are present.");
#endif

    create_info->enabledLayerCount = required_validation_layer_count;
    create_info->ppEnabledLayerNames = required_validation_layer_names;
    //END Validation layers.
    return TRUE;
}


void vulkan_setup_debugger(){
    // Debugger
#if defined(_DEBUG)
    PRINT_DEBUG("Creating Vulkan debugger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
                                                                      //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    YASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    PRINT_DEBUG("Vulkan debugger created.");
#endif
}