#pragma once

#include "renderer/webgpu/webgpu_types.inl"
#include "renderer/renderer_types.inl"

b8 webgpu_ui_shader_create(WEBGPU_CONTEXT* context, WEBGPU_UI_SHADER* out_shader);

void webgpu_ui_shader_destroy(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader);

void webgpu_ui_shader_use(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader);

void webgpu_ui_shader_update_global_state(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader, f32 delta_time);

void webgpu_ui_shader_set_model(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader, Matrice4 model);
void webgpu_ui_shader_apply_material(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader, MATERIAL* material);

b8 webgpu_ui_shader_acquire_resources(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader, MATERIAL* material);
void webgpu_ui_shader_release_resources(WEBGPU_CONTEXT* context, struct WEBGPU_UI_SHADER* shader, MATERIAL* material);
