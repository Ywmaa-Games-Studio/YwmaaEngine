#include "webgpu_types.inl"

b8 webgpu_pipeline_create(WEBGPU_CONTEXT* context,
    u32 bind_group_layout_count,
    WGPUBindGroupLayout* bind_group_layouts,
    WGPUVertexState* vertex_stage,
    WGPUFragmentState* fragment_stage,
    u32 push_constant_range_count,
    range* push_constant_ranges,
    b8 is_wireframe,
    b8 depth_test_enabled,
    WEBGPU_PIPELINE* pipeline);

void webgpu_pipeline_destroy(WEBGPU_CONTEXT* context, WEBGPU_PIPELINE* pipeline);
