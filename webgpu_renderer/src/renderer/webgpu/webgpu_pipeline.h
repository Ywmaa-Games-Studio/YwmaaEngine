#include "webgpu_types.inl"

b8 webgpu_pipeline_create(WEBGPU_CONTEXT* context,
    const WEBGPU_PIPELINE_CONFIG* config,
    WEBGPU_PIPELINE* pipeline);

void webgpu_pipeline_destroy(WEBGPU_CONTEXT* context, WEBGPU_PIPELINE* pipeline);
