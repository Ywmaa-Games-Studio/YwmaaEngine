#pragma once

#include "vulkan_types.inl"

void vulkan_swapchain_create(
    VULKAN_CONTEXT* context,
    u32 width,
    u32 height,
    VULKAN_SWAPCHAIN* out_swapchain);

void vulkan_swapchain_recreate(
    VULKAN_CONTEXT* context,
    u32 width,
    u32 height,
    VULKAN_SWAPCHAIN* swapchain);

void vulkan_swapchain_destroy(
    VULKAN_CONTEXT* context,
    VULKAN_SWAPCHAIN* swapchain);

b8 vulkan_swapchain_acquire_next_image_index(
    VULKAN_CONTEXT* context,
    VULKAN_SWAPCHAIN* swapchain,
    u64 timeout_ns,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* out_image_index);

void vulkan_swapchain_present(
    VULKAN_CONTEXT* context,
    VULKAN_SWAPCHAIN* swapchain,
    VkQueue graphics_queue,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 present_image_index);
