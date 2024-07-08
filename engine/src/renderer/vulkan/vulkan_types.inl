#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <vulkan/vulkan.h>

// Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)               \
    {                                \
        YASSERT(expr == VK_SUCCESS); \
    }

typedef struct VULKAN_CONTEXT {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} VULKAN_CONTEXT;