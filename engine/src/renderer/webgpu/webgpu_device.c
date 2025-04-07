#include "webgpu_device.h"
#include "core/logger.h"
#include "core/asserts.h"

b8 webgpu_device_create(WEBGPU_CONTEXT* context){

    //START Device creation
    //Adapter
    PRINT_DEBUG("Requesting adapter...");

    WGPURequestAdapterOptions adapter_opts = {};
    adapter_opts.nextInChain = NULL;
    adapter_opts.compatibleSurface = context->surface;
    context->adapter = request_adapter_sync(context->instance, &adapter_opts);

    PRINT_INFO("Got adapter: %i", context->adapter);

#ifdef _DEBUG
    WGPUAdapterInfo properties = {};
    properties.nextInChain = NULL;
    wgpuAdapterGetInfo(context->adapter, &properties);
    PRINT_INFO("Adapter properties:");
    PRINT_INFO(" - vendorID: %i", properties.vendorID);
    if (properties.vendor) {
        PRINT_INFO(" - vendorName: %s", properties.vendor);
    }
    if (properties.architecture) {
        PRINT_INFO(" - architecture: %s", properties.architecture);
    }
    PRINT_INFO(" - deviceID: %i", properties.deviceID);
    if (properties.device) {
        PRINT_INFO(" - name: %s", properties.device);
    }
    if (properties.description) {
        PRINT_INFO(" - driverDescription: %s", properties.description);
    }
    PRINT_INFO(" - adapterType: 0x%i", properties.adapterType);
    PRINT_INFO(" - backendType: 0x%i", properties.backendType);

#endif

    WGPURequiredLimits required_limits = get_required_limits(context);
    // Device
    PRINT_DEBUG("Requesting device...");
    WGPUDeviceDescriptor device_desc = {};
    device_desc.label = "My Device"; // anything works here, that's your call
    device_desc.requiredFeatureCount = 0; // we do not require any specific feature
    device_desc.requiredLimits = NULL;//&required_limits; // this crashes, see: https://github.com/eliemichel/WebGPU-distribution/issues/38
    device_desc.defaultQueue.nextInChain = NULL;
    device_desc.defaultQueue.label = "The default queue";
    device_desc.deviceLostCallback = on_device_lost;
    // [...] Build device descriptor
    context->device = request_device_sync(context->adapter, &device_desc);

    //wgpuDeviceSetUncapturedErrorCallback(context->device, on_device_error, NULL /* pUserData */);
    PRINT_INFO("Got device: %i", context->device);

	WGPUSupportedLimits device_supported_limits;
    wgpuDeviceGetLimits(context->device, &device_supported_limits);
    PRINT_INFO("device.maxVertexAttributes: %i", device_supported_limits.limits.maxVertexAttributes);

    //context->swapchain_format = wgpuSurfaceGetPreferredFormat(context->surface, context->adapter); This changed to the code below
    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities( context->surface, context->adapter, &capabilities );
    context->swapchain_format = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers( capabilities );
    //END Device creation

    context->queue = wgpuDeviceGetQueue(context->device);

    PRINT_INFO("Queues obtained.");
    wgpuQueueOnSubmittedWorkDone(context->queue, on_queue_work_done, NULL /* pUserData */);

    PRINT_DEBUG("Destroying WebGPU Adapter...");
    wgpuAdapterRelease(context->adapter);
    
    return context->device != NULL;
}
void webgpu_device_destroy(WEBGPU_CONTEXT* context){
    PRINT_DEBUG("Destroying WebGPU Queue...");
    wgpuQueueRelease(context->queue);

    PRINT_DEBUG("Destroying WebGPU Device...");
    wgpuDeviceRelease(context->device);
}

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
// A simple structure holding the local information shared with the
// onAdapterRequestEnded callback.
struct adapter_request_data {
    WGPUAdapter adapter;
    b8 requestEnded;
};
WGPUAdapter request_adapter_sync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
    struct adapter_request_data adapter_data;

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        on_adapter_request_ended,
        &adapter_data
    );

    // We wait until userData.requestEnded gets true
    // [...] Wait for request to end
    YASSERT(adapter_data.requestEnded);

    return adapter_data.adapter;
}

// Callback called by wgpuInstanceRequestAdapter when the request returns
// This is a C++ lambda function, but could be any function defined in the
// global scope. It must be non-capturing (the brackets [] are empty) so
// that it behaves like a regular C function pointer, which is what
// wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
// is to convey what we want to capture through the pUserData pointer,
// provided as the last argument of wgpuInstanceRequestAdapter and received
// by the callback as its last argument.
void on_adapter_request_ended(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
    struct adapter_request_data* adapter_data = (pUserData);
    if (status == WGPURequestAdapterStatus_Success) {
        adapter_data->adapter = adapter;
    } else {
        PRINT_ERROR("Could not get WebGPU adapter: ", message);
    }
    adapter_data->requestEnded = true;
};




/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
struct device_request_data {
    WGPUDevice device;
    b8 requestEnded;
};

WGPUDevice request_device_sync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
    struct device_request_data device_data;

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        on_device_request_ended,
        &device_data
    );

#ifdef __EMSCRIPTEN__
    while (!device_data.requestEnded) {
        emscripten_sleep(100);
    }
#endif // __EMSCRIPTEN__

    YASSERT(device_data.requestEnded);

    return device_data.device;
}

void on_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
    struct device_request_data* device_data = (pUserData);
    if (status == WGPURequestDeviceStatus_Success) {
        device_data->device = device;
    } else {
        PRINT_ERROR("Could not get WebGPU device: ", message);
    }
    device_data->requestEnded = true;
};



void on_device_lost(WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
    PRINT_ERROR("Device lost: reason ", reason);
    if (message) PRINT_ERROR("message: ", message);
};

void on_device_error(WGPUErrorType type, char const* message, void* /* pUserData */) {
    PRINT_ERROR("Uncaptured device error: type %i", type);
    if (message) PRINT_ERROR("message: %s", message);
};



void on_queue_work_done(WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
    PRINT_DEBUG("Queued work finished with status: ", status);
};

void webgpu_set_default(WGPULimits limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    // [...] Set everything to WGPU_LIMIT_U32_UNDEFINED or WGPU_LIMIT_U64_UNDEFINED to mean no limit
}

WGPURequiredLimits get_required_limits(WEBGPU_CONTEXT* context) {
    // Get adapter supported limits, in case we need them
	WGPUSupportedLimits adapter_supported_limits;
	wgpuAdapterGetLimits(context->adapter, &adapter_supported_limits);
    PRINT_INFO("adapter.maxVertexAttributes: %i", adapter_supported_limits.limits.maxVertexAttributes);

    WGPURequiredLimits requiredLimits;
    webgpu_set_default(requiredLimits.limits);

    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 2;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    // Maximum size of a buffer is 6 vertices of Vertex3D each
    requiredLimits.limits.maxBufferSize = 32 * sizeof(Vertex3D);
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(Vertex3D);

    // [...] Other device limits

    return requiredLimits;
}