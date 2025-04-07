#pragma once

#include "renderer/webgpu/webgpu_types.inl"
#include "renderer/renderer_types.inl"

b8 webgpu_object_shader_create(WEBGPU_CONTEXT* context, WEBGPU_OBJECT_SHADER* out_shader);

void webgpu_object_shader_destroy(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader);

void webgpu_object_shader_use(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader);

void webgpu_object_shader_update_global_state(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, f32 delta_time);

void webgpu_object_shader_update_object(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, GEOMETRY_RENDER_DATA data);

b8 webgpu_object_shader_acquire_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32* out_object_id);
void webgpu_object_shader_release_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32 object_id);