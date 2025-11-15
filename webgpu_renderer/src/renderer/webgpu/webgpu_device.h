#include "webgpu_types.inl"

b8 webgpu_device_create(WEBGPU_CONTEXT* context);
void webgpu_device_destroy(WEBGPU_CONTEXT* context);

WGPUAdapter request_adapter_sync(WGPUInstance instance, WGPURequestAdapterOptions const * options);
void on_adapter_request_ended(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2);

WGPUDevice request_device_sync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);
void on_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2);
void on_device_lost(WGPUDevice const * device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2);
void on_device_error(WGPUErrorType type, char const* message, void* /* pUserData */);

void on_queue_work_done(WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2);

WGPUNativeLimits get_required_limits(WEBGPU_CONTEXT* context);
b8 webgpu_device_detect_depth_format(WEBGPU_CONTEXT* context, b8 need_stencil);
