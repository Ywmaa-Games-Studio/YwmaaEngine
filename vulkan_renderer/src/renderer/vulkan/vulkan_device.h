#pragma once

#include "vulkan_types.inl"

b8 vulkan_device_create(VULKAN_CONTEXT* context);

void vulkan_device_destroy(VULKAN_CONTEXT* context);

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    VULKAN_SWAPCHAIN_SUPPORT_INFO* out_support_info);

b8 vulkan_device_detect_depth_format(VULKAN_DEVICE* device);
