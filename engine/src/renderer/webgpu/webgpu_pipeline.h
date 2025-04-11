#include "webgpu_types.inl"

b8 webgpu_pipeline_create(WEBGPU_CONTEXT* context, WGPUVertexBufferLayout vertex_buffer_layout, WEBGPU_MATERIAL_SHADER* shader);
void webgpu_pipeline_destroy(WEBGPU_CONTEXT* context, WEBGPU_MATERIAL_SHADER* shader);