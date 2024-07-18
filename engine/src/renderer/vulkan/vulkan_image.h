#pragma once

#include "vulkan_types.inl"

void vulkan_image_create(
    VULKAN_CONTEXT* context,
    VkImageType image_type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags,
    b32 create_view,
    VkImageAspectFlags view_aspect_flags,
    VULKAN_IMAGE* out_image);

void vulkan_image_view_create(
    VULKAN_CONTEXT* context,
    VkFormat format,
    VULKAN_IMAGE* image,
    VkImageAspectFlags aspect_flags);

void vulkan_image_destroy(VULKAN_CONTEXT* context, VULKAN_IMAGE* image);