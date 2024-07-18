#pragma once

#include "defines.h"
#include "core/asserts.h"

#include "../thirdparty/volk/volk.h"

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

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;
} VULKAN_DEVICE;

typedef struct VULKAN_IMAGE {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} VULKAN_IMAGE;

typedef enum VULKAN_RENDER_PASS_STATE {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
} VULKAN_RENDER_PASS_STATE;

typedef struct VULKAN_RENDERPASS {
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VULKAN_RENDER_PASS_STATE state;
} VULKAN_RENDERPASS;

typedef struct VULKAN_SWAPCHAIN {
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR handle;
    u32 image_count;
    VkImage* images;
    VkImageView* views;

    VULKAN_IMAGE depth_attachment;
} VULKAN_SWAPCHAIN;

typedef enum VULKAN_COMMAND_BUFFER_STATE {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} VULKAN_COMMAND_BUFFER_STATE;

typedef struct VULKAN_COMMAND_BUFFER {
    VkCommandBuffer handle;

    // Command buffer state.
    VULKAN_COMMAND_BUFFER_STATE state;
} VULKAN_COMMAND_BUFFER;
typedef struct VULKAN_CONTEXT {

    // The framebuffer's current width.
    u32 framebuffer_width;

    // The framebuffer's current height.
    u32 framebuffer_height;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VULKAN_DEVICE device;

    VULKAN_SWAPCHAIN swapchain;
    VULKAN_RENDERPASS main_renderpass;
    u32 image_index;
    u32 current_frame;

    b8 recreating_swapchain;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} VULKAN_CONTEXT;