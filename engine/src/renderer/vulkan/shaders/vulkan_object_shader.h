#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

b8 vulkan_object_shader_create(VULKAN_CONTEXT* context, VULKAN_OBJECT_SHADER* out_shader);

void vulkan_object_shader_destroy(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader);

void vulkan_object_shader_use(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader);

void vulkan_object_shader_update_global_state(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader, f32 delta_time);

void vulkan_object_shader_update_object(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader, GEOMETRY_RENDER_DATA data);

b8 vulkan_object_shader_acquire_resources(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader, u32* out_object_id);
void vulkan_object_shader_release_resources(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader, u32 object_id);