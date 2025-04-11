#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

b8 vulkan_material_shader_create(VULKAN_CONTEXT* context, VULKAN_MATERIAL_SHADER* out_shader);

void vulkan_material_shader_destroy(VULKAN_CONTEXT* context, struct VULKAN_MATERIAL_SHADER* shader);

void vulkan_material_shader_use(VULKAN_CONTEXT* context, struct VULKAN_MATERIAL_SHADER* shader);

void vulkan_material_shader_update_global_state(VULKAN_CONTEXT* context, struct VULKAN_MATERIAL_SHADER* shader, f32 delta_time);

void vulkan_material_shader_update_object(VULKAN_CONTEXT* context, struct VULKAN_MATERIAL_SHADER* shader, GEOMETRY_RENDER_DATA data);

b8 vulkan_material_shader_acquire_resources(VULKAN_CONTEXT* context, struct VULKAN_MATERIAL_SHADER* shader, u32* out_object_id);
void vulkan_material_shader_release_resources(VULKAN_CONTEXT* context, struct VULKAN_MATERIAL_SHADER* shader, u32 object_id);