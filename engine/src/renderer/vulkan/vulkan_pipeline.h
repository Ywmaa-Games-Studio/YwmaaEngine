#pragma once

#include "vulkan_types.inl"

b8 vulkan_graphics_pipeline_create(
    VULKAN_CONTEXT* context,
    VULKAN_RENDERPASS* renderpass,
    u32 attribute_count,
    VkVertexInputAttributeDescription* attributes,
    u32 descriptor_set_layout_count,
    VkDescriptorSetLayout* descriptor_set_layouts,
    u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    b8 is_wireframe,
    VULKAN_PIPELINE* out_pipeline);

void vulkan_pipeline_destroy(VULKAN_CONTEXT* context, VULKAN_PIPELINE* pipeline);

void vulkan_pipeline_bind(VULKAN_COMMAND_BUFFER* command_buffer, VkPipelineBindPoint bind_point, VULKAN_PIPELINE* pipeline);
