#pragma once

#include "vulkan_types.inl"

b8 vulkan_graphics_pipeline_create(VULKAN_CONTEXT* context, const VULKAN_PIPELINE_CONFIG* config, VULKAN_PIPELINE* out_pipeline);

void vulkan_pipeline_destroy(VULKAN_CONTEXT* context, VULKAN_PIPELINE* pipeline);

void vulkan_pipeline_bind(VULKAN_COMMAND_BUFFER* command_buffer, VkPipelineBindPoint bind_point, VULKAN_PIPELINE* pipeline);
