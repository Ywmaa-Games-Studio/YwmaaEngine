#pragma once

#include "vulkan_types.inl"

void vulkan_renderpass_create(
    VULKAN_CONTEXT* context, 
    VULKAN_RENDERPASS* out_renderpass,
    f32 x, f32 y, f32 w, f32 h,
    f32 r, f32 g, f32 b, f32 a,
    f32 depth,
    u32 stencil);

void vulkan_renderpass_destroy(VULKAN_CONTEXT* context, VULKAN_RENDERPASS* renderpass);

void vulkan_renderpass_begin(
    VULKAN_COMMAND_BUFFER* command_buffer, 
    VULKAN_RENDERPASS* renderpass,
    VkFramebuffer frame_buffer);

void vulkan_renderpass_end(VULKAN_COMMAND_BUFFER* command_buffer, VULKAN_RENDERPASS* renderpass);