#pragma once

#include "vulkan_types.inl"

typedef enum E_RENDERPASS_CLEAR_FLAG {
    RENDERPASS_CLEAR_NONE_FLAG = 0x0,
    RENDERPASS_CLEAR_COLOUR_BUFFER_FLAG = 0x1,
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG = 0x2,
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4
} E_RENDERPASS_CLEAR_FLAG;

void vulkan_renderpass_create(
    VULKAN_CONTEXT* context, 
    VULKAN_RENDERPASS* out_renderpass,
    Vector4 render_area,
    Vector4 clear_colour,
    f32 depth,
    u32 stencil,
    u8 clear_flags,
    b8 has_prev_pass,
    b8 has_next_pass);

void vulkan_renderpass_destroy(VULKAN_CONTEXT* context, VULKAN_RENDERPASS* renderpass);

void vulkan_renderpass_begin(
    VULKAN_COMMAND_BUFFER* command_buffer, 
    VULKAN_RENDERPASS* renderpass,
    VkFramebuffer frame_buffer);

void vulkan_renderpass_end(VULKAN_COMMAND_BUFFER* command_buffer, VULKAN_RENDERPASS* renderpass);
