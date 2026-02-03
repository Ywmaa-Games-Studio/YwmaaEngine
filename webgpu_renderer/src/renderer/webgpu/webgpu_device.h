#include "webgpu_types.inl"

b8 webgpu_device_create(WEBGPU_CONTEXT* context);
void webgpu_device_destroy(WEBGPU_CONTEXT* context);

b8 webgpu_device_detect_depth_format(WEBGPU_CONTEXT* context, b8 need_stencil);

