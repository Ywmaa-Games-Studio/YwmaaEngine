#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

b8 vulkan_ui_shader_create(VULKAN_CONTEXT* context, VULKAN_UI_SHADER* out_shader);

void vulkan_ui_shader_destroy(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader);

void vulkan_ui_shader_use(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader);

void vulkan_ui_shader_update_global_state(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader, f32 delta_time);

void vulkan_ui_shader_set_model(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader, Matrice4 model);
void vulkan_ui_shader_apply_material(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader, MATERIAL* material);

b8 vulkan_ui_shader_acquire_resources(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader, MATERIAL* material);
void vulkan_ui_shader_release_resources(VULKAN_CONTEXT* context, struct VULKAN_UI_SHADER* shader, MATERIAL* material);
