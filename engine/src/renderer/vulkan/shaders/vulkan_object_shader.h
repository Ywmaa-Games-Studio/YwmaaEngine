#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

b8 vulkan_object_shader_create(VULKAN_CONTEXT* context, VULKAN_OBJECT_SHADER* out_shader);

void vulkan_object_shader_destroy(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader);

void vulkan_object_shader_use(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader);