#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <vulkan/vulkan.h>

// Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)               \
    {                                \
        YASSERT(expr == VK_SUCCESS); \
    }

typedef struct VULKAN_SWAPCHAIN_SUPPORT_INFO {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} VULKAN_SWAPCHAIN_SUPPORT_INFO;

typedef struct VULKAN_DEVICE {
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    VULKAN_SWAPCHAIN_SUPPORT_INFO swapchain_support;
    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
} VULKAN_DEVICE;
typedef struct VULKAN_CONTEXT {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VULKAN_DEVICE device;
#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} VULKAN_CONTEXT;