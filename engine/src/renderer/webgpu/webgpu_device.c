#include "webgpu_device.h"
#include "core/logger.h"
#include "core/asserts.h"

// const backend strings for debug

static const char* backend_type_to_string(WGPUBackendType backend_type) {
    switch (backend_type) {
        case WGPUBackendType_Null: return "Null";
        case WGPUBackendType_WebGPU: return "WebGPU";
        case WGPUBackendType_D3D11: return "D3D11";
        case WGPUBackendType_D3D12: return "D3D12";
        case WGPUBackendType_Metal: return "Metal";
        case WGPUBackendType_Vulkan: return "Vulkan";
        case WGPUBackendType_OpenGL: return "OpenGL";
        case WGPUBackendType_OpenGLES: return "OpenGLES";
        default: return "Unknown";
    }
}

// const adapter type strings for debug
static const char* adapter_type_to_string(WGPUAdapterType adapter_type) {
    switch (adapter_type) {
        case WGPUAdapterType_DiscreteGPU: return "Discrete GPU";
        case WGPUAdapterType_IntegratedGPU: return "Integrated GPU";
        case WGPUAdapterType_CPU: return "CPU";
        case WGPUAdapterType_Unknown: return "Unknown";
        default: return "Unknown";
    }
}


b8 webgpu_device_create(WEBGPU_CONTEXT* context){

    //START Device creation
    //Adapter
    PRINT_DEBUG("Requesting adapter...");

    WGPURequestAdapterOptions adapter_opts = {0};
    //adapter_opts.backendType = WGPUBackendType_OpenGL;
    // TODO: make this configurable
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapter_opts.featureLevel = WGPUFeatureLevel_Core;
    //adapter_opts.forceFallbackAdapter = true;
    adapter_opts.nextInChain = NULL;
    adapter_opts.compatibleSurface = context->surface;
    context->adapter = request_adapter_sync(context->instance, &adapter_opts);

    PRINT_INFO("Got adapter: %i", context->adapter);

    WGPUAdapterInfo properties = {0};
    properties.nextInChain = NULL;
    wgpuAdapterGetInfo(context->adapter, &properties);
    PRINT_INFO("Adapter properties:");
#ifdef _DEBUG
    PRINT_INFO(" - Vendor ID: %i", properties.vendorID);
#endif
    if (properties.vendor.data) {
        PRINT_INFO(" - Vendor Name: %s", properties.vendor.data);
    }
    if (properties.architecture.data) {
        PRINT_INFO(" - Architecture: %s", properties.architecture.data);
    }
#ifdef _DEBUG
    PRINT_INFO(" - Device ID: %i", properties.deviceID);
#endif
    if (properties.device.data) {
        PRINT_INFO(" - Name: %s", properties.device.data);
    }
    if (properties.description.data) {
        PRINT_INFO(" - Driver Description: %s", properties.description.data);
    }
    PRINT_INFO(" - Adapter Type: %s", adapter_type_to_string(properties.adapterType) );
    PRINT_INFO(" - Backend Type: %s",  backend_type_to_string(properties.backendType) );

    
    


/*     WGPUNativeLimits required_limits_extras = get_required_limits(context);
    required_limits_extras.chain.sType = WGPUSType_NativeLimits; */

    WGPUNativeLimits required_limits_extras = {
      .maxPushConstantSize = 128,
    };
    required_limits_extras.chain.next = NULL;
    required_limits_extras.chain.sType = (WGPUSType)WGPUSType_NativeLimits;

    WGPUNativeFeature required_features[] = {
        WGPUNativeFeature_PushConstants
    };

    WGPULimits required_limits = {0};
    wgpuAdapterGetLimits(context->adapter, &required_limits);
    required_limits.nextInChain = &required_limits_extras.chain;
    // Device
    PRINT_DEBUG("Requesting device...");
    WGPUDeviceDescriptor device_desc = {0};
    char *device_name = "WebGPU Device";
    device_desc.label = (WGPUStringView){device_name, sizeof(device_name)};
    device_desc.requiredFeatureCount = 1;
    device_desc.requiredFeatures = (WGPUFeatureName*)required_features;
    device_desc.requiredLimits = &required_limits;
    device_desc.defaultQueue.nextInChain = NULL;
    device_desc.defaultQueue.label = (WGPUStringView){"The default queue", sizeof("The default queue")};
    device_desc.deviceLostCallbackInfo = (WGPUDeviceLostCallbackInfo){NULL, WGPUCallbackMode_AllowSpontaneous, on_device_lost};
    // [...] Build device descriptor
    context->device = request_device_sync(context->adapter, &device_desc);

    wgpuDeviceGetLimits(context->device, &context->device_supported_limits);
    PRINT_INFO("Got device: %i", context->device);
#ifdef _DEBUG
    PRINT_TRACE("device.maxVertexAttributes: %i", context->device_supported_limits.maxVertexAttributes);
    PRINT_TRACE("device.minUniformBufferOffsetAlignment: %i", context->device_supported_limits.minUniformBufferOffsetAlignment);
    PRINT_TRACE("device.maxBindGroups: %i", context->device_supported_limits.maxBindGroups);
    PRINT_TRACE("device.maxVertexBufferArrayStride: %i", context->device_supported_limits.maxVertexBufferArrayStride);
    PRINT_TRACE("device.maxUniformBuffersPerShaderStage: %i", context->device_supported_limits.maxUniformBuffersPerShaderStage);
    PRINT_TRACE("device.maxUniformBufferBindingSize: %i", context->device_supported_limits.maxUniformBufferBindingSize);
    PRINT_TRACE("device.maxSamplersPerShaderStage: %i", context->device_supported_limits.maxSamplersPerShaderStage);
    PRINT_TRACE("device.maxSampledTexturesPerShaderStage: %i", context->device_supported_limits.maxSampledTexturesPerShaderStage);
#endif
    //context->swapchain_format = wgpuSurfaceGetPreferredFormat(context->surface, context->adapter); This changed to the code below
    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities( context->surface, context->adapter, &capabilities );
    //context->swapchain_format = capabilities.formats[0];
    // currently use linear format
    context->swapchain_format = WGPUTextureFormat_RGBA8Unorm;
    wgpuSurfaceCapabilitiesFreeMembers( capabilities );
    //END Device creation

    context->queue = wgpuDeviceGetQueue(context->device);

    PRINT_INFO("Queues obtained.");
    wgpuQueueOnSubmittedWorkDone(context->queue, (WGPUQueueWorkDoneCallbackInfo) {NULL, WGPUCallbackMode_AllowSpontaneous, on_queue_work_done});
#ifdef _DEBUG
    PRINT_DEBUG("Destroying WebGPU Adapter...");
#endif
    wgpuAdapterRelease(context->adapter);
    
    return context->device != NULL;
}
void webgpu_device_destroy(WEBGPU_CONTEXT* context){
    if (!context) {
        PRINT_ERROR("webgpu_device_destroy called with NULL context");
        return;
    }
    if (context->queue) {
        PRINT_DEBUG("Destroying WebGPU Queue...");
        wgpuQueueRelease(context->queue);
    }
    if (context->device) {
        PRINT_DEBUG("Destroying WebGPU Device...");
        wgpuDeviceRelease(context->device);
    }
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
        (WGPURequestAdapterCallbackInfo){NULL, WGPUCallbackMode_AllowProcessEvents, on_adapter_request_ended, &adapter_data}
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
void on_adapter_request_ended(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2 ) {
    struct adapter_request_data* adapter_data = (userdata1);
    if (status == WGPURequestAdapterStatus_Success) {
        adapter_data->adapter = adapter;
    } else {
        PRINT_ERROR("Could not get WebGPU adapter: ", message);
    }
    adapter_data->requestEnded = true;
}




/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
struct device_request_data {
    WGPUDevice device;
    b8 request_ended;
    b8 success;
};

WGPUDevice request_device_sync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
    struct device_request_data device_data;

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        (WGPURequestDeviceCallbackInfo) {NULL, WGPUCallbackMode_AllowProcessEvents, on_device_request_ended, &device_data}
    );

#ifdef __EMSCRIPTEN__
    while (!device_data.requestEnded) {
        emscripten_sleep(100);
    }
#endif // __EMSCRIPTEN__

    YASSERT_MSG(device_data.success, "Failed to get WGPU Device");
    

    return device_data.device;
}

void on_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
    struct device_request_data* device_data = (userdata1);
    if (status == WGPURequestDeviceStatus_Success) {
        device_data->device = device;
        device_data->success = true;
    } else {
        PRINT_ERROR("Could not get WebGPU device: %s", message.data);
        device_data->success = false;
    }
    device_data->request_ended = true;
}



void on_device_lost(WGPUDevice const * device, WGPUDeviceLostReason reason, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
    PRINT_ERROR("Device lost: reason ", reason);
    if (message.data) PRINT_ERROR("message: ", message.data);
}

void on_device_error(WGPUErrorType type, char const* message, void* pUserData) {
    PRINT_ERROR("Uncaptured device error: type %i", type);
    if (message) PRINT_ERROR("message: %s", message);
}



void on_queue_work_done(WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
    PRINT_DEBUG("Queued work finished with status: ", status);
}

void webgpu_set_default(WGPULimits limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    // [...] Set everything to WGPU_LIMIT_U32_UNDEFINED or WGPU_LIMIT_U64_UNDEFINED to mean no limit
}

WGPUNativeLimits get_required_limits(WEBGPU_CONTEXT* context) {
    // Get adapter supported limits, in case we need them
	WGPULimits adapter_supported_limits;
	wgpuAdapterGetLimits(context->adapter, &adapter_supported_limits);
    PRINT_INFO("adapter.maxVertexAttributes: %i", adapter_supported_limits.maxVertexAttributes);

    WGPUNativeLimits requiredLimits;

    // We use at most 1 vertex attribute for now
    //requiredLimits.maxNonSamplerBindings = 5;
    requiredLimits.maxPushConstantSize = 0;

    // [...] Other device limits

    return requiredLimits;
}
