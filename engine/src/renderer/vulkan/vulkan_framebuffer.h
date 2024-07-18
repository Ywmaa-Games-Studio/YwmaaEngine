#pragma once

#include "vulkan_types.inl"

void vulkan_framebuffer_create(
    VULKAN_CONTEXT* context,
    VULKAN_RENDERPASS* renderpass,
    u32 width,
    u32 height,
    u32 attachment_count,
    VkImageView* attachments,
    VULKAN_FRAMEBUFFER* out_framebuffer);

void vulkan_framebuffer_destroy(VULKAN_CONTEXT* context, VULKAN_FRAMEBUFFER* framebuffer);