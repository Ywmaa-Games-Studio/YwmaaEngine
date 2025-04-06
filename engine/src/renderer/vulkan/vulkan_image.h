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

/**
 * Transitions the provided image from old_layout to new_layout.
 */
void vulkan_image_transition_layout(
    VULKAN_CONTEXT* context,
    VULKAN_COMMAND_BUFFER* command_buffer,
    VULKAN_IMAGE* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout);

/**
 * Copies data in buffer to provided image.
 * @param context The Vulkan context.
 * @param image The image to copy the buffer's data to.
 * @param buffer The buffer whose data will be copied.
 */
void vulkan_image_copy_from_buffer(
    VULKAN_CONTEXT* context,
    VULKAN_IMAGE* image,
    VkBuffer buffer,
    VULKAN_COMMAND_BUFFER* command_buffer);


void vulkan_image_destroy(VULKAN_CONTEXT* context, VULKAN_IMAGE* image);