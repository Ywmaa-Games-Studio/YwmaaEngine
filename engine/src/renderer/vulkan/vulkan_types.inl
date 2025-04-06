#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "renderer/renderer_types.inl"

#include "../thirdparty/volk/volk.h"

// Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)               \
    {                                \
        YASSERT(expr == VK_SUCCESS); \
    }
typedef struct VULKAN_BUFFER {
    u64 total_size;
    VkBuffer handle;
    VkBufferUsageFlagBits usage;
    b8 is_locked;
    VkDeviceMemory memory;
    i32 memory_index;
    u32 memory_property_flags;
} VULKAN_BUFFER;
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

    VkCommandPool graphics_command_pool;

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


typedef struct VULKAN_FRAMEBUFFER {
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView* attachments;
    VULKAN_RENDERPASS* renderpass;
} VULKAN_FRAMEBUFFER;
typedef struct VULKAN_SWAPCHAIN {
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR handle;
    u32 image_count;
    VkImage* images;
    VkImageView* views;

    VULKAN_IMAGE depth_attachment;

    // framebuffers used for on-screen rendering.
    VULKAN_FRAMEBUFFER* framebuffers;
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

typedef struct VULKAN_FENCE {
    VkFence handle;
    b8 is_signaled;
} VULKAN_FENCE;
typedef struct VULKAN_SHADER_STAGE {
    VkShaderModuleCreateInfo create_info;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo shader_stage_create_info;
} VULKAN_SHADER_STAGE;

typedef struct VULKAN_PIPELINE {
    VkPipeline handle;
    VkPipelineLayout pipeline_layout;
} VULKAN_PIPELINE;

#define OBJECT_SHADER_STAGE_COUNT 2
typedef struct VULKAN_OBJECT_SHADER {
    // vertex, fragment
    VULKAN_SHADER_STAGE stages[OBJECT_SHADER_STAGE_COUNT];

    VkDescriptorPool global_descriptor_pool;
    VkDescriptorSetLayout global_descriptor_set_layout;

    // One descriptor set per frame - max 3 for triple-buffering.
    VkDescriptorSet global_descriptor_sets[3];
    b8 descriptor_updated[3];

    // Global uniform object.
    GLOBAL_UNIFORM_OBJECT global_ubo;

    // Global uniform buffer.
    VULKAN_BUFFER global_uniform_buffer;

    VULKAN_PIPELINE pipeline;


} VULKAN_OBJECT_SHADER;
typedef struct VULKAN_CONTEXT {

    // The framebuffer's current width.
    u32 framebuffer_width;

    // The framebuffer's current height.
    u32 framebuffer_height;

    // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation,
    // a new one should be generated.
    u64 framebuffer_size_generation;

    // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation
    // when updated.
    u64 framebuffer_size_last_generation;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VULKAN_DEVICE device;

    VULKAN_SWAPCHAIN swapchain;
    VULKAN_RENDERPASS main_renderpass;

    VULKAN_BUFFER object_vertex_buffer;
    VULKAN_BUFFER object_index_buffer;

    // darray
    VULKAN_COMMAND_BUFFER* graphics_command_buffers;

    // darray
    VkSemaphore* image_available_semaphores;

    // darray
    VkSemaphore* queue_complete_semaphores;

    u32 in_flight_fence_count;
    VULKAN_FENCE* in_flight_fences;

    // Holds pointers to fences which exist and are owned elsewhere.
    VULKAN_FENCE** images_in_flight;

    u32 image_index;
    u32 current_frame;

    b8 recreating_swapchain;

    VULKAN_OBJECT_SHADER object_shader;

    u64 geometry_vertex_offset;
    u64 geometry_index_offset;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} VULKAN_CONTEXT;