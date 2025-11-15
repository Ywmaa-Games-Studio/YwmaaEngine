#pragma once

#include "vulkan_types.inl"

b8 create_shader_module(
    VULKAN_CONTEXT* context,
    const char* name,
    const char* type_str,
    VkShaderStageFlagBits shader_stage_flag,
    u32 stage_index,
    VULKAN_SHADER_STAGE* shader_stages
);
