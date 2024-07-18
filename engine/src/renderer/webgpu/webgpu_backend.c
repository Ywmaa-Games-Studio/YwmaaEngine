#include "webgpu_backend.h"
#include "webgpu_platform.h"

#include "webgpu_types.inl"
#include "core/logger.h"
#include "core/ystring.h"
#include "core/asserts.h"

#include "variants/darray.h"

#include "platform/platform.h"

WGPUAdapter request_adapter_sync(WGPUInstance instance, WGPURequestAdapterOptions const * options);
void on_adapter_request_ended(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData);

WGPUDevice request_device_sync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);
void on_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData);
void on_device_lost(WGPUDeviceLostReason reason, char const* message, void* /* pUserData */);
void on_device_error(WGPUErrorType type, char const* message, void* /* pUserData */);
// static Vulkan context
static WEBGPU_CONTEXT context;

b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name, struct PLATFORM_STATE* platform_state) {
    // We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = NULL;

// We create the instance using this descriptor
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    PRINT_INFO("Detected Web Build Use EMSCRIPTEN.");
    context.instance = wgpuCreateInstance(nullptr);
#else //  WEBGPU_BACKEND_EMSCRIPTEN
    context.instance = wgpuCreateInstance(&desc);
#endif //  WEBGPU_BACKEND_EMSCRIPTEN


    // We can check whether there is actually an instance created
    if (!context.instance) {
        PRINT_ERROR("Could not initialize WebGPU!");
        return FALSE;
    }
    PRINT_INFO("WGPU instance: %i", context.instance);

    //START Surface
    PRINT_DEBUG("Creating WebGPU surface...");
    if (!platform_create_webgpu_surface(platform_state, &context)) {
        PRINT_ERROR("Failed to create platform surface!");
        return FALSE;
    }
    PRINT_DEBUG("WebGPU surface created.");
    //END

    //START Device creation
    //Adapter
    PRINT_DEBUG("Requesting adapter...");

    WGPURequestAdapterOptions adapter_opts = {};
    adapter_opts.nextInChain = NULL;
    adapter_opts.compatibleSurface = context.surface;
    context.adapter = request_adapter_sync(context.instance, &adapter_opts);

    PRINT_INFO("Got adapter: %i", context.adapter);
    WGPUAdapterProperties properties = {};
    properties.nextInChain = NULL;
    wgpuAdapterGetProperties(context.adapter, &properties);
    PRINT_INFO("Adapter properties:");
    PRINT_INFO(" - vendorID: ", properties.vendorID);
    if (properties.vendorName) {
        PRINT_INFO(" - vendorName: ", properties.vendorName);
    }
    if (properties.architecture) {
        PRINT_INFO(" - architecture: ", properties.architecture);
    }
    PRINT_INFO(" - deviceID: ", properties.deviceID);
    if (properties.name) {
        PRINT_INFO(" - name: ", properties.name);
    }
    if (properties.driverDescription) {
        PRINT_INFO(" - driverDescription: ", properties.driverDescription);
    }
    PRINT_INFO(" - adapterType: 0x", properties.adapterType);
    PRINT_INFO(" - backendType: 0x", properties.backendType);
    // Device
    PRINT_DEBUG("Requesting device...");

    WGPUDeviceDescriptor device_desc = {};
    device_desc.label = "My Device"; // anything works here, that's your call
    device_desc.requiredFeatureCount = 0; // we do not require any specific feature
    device_desc.requiredLimits = NULL; // we do not require any specific limit
    device_desc.defaultQueue.nextInChain = NULL;
    device_desc.defaultQueue.label = "The default queue";
    device_desc.deviceLostCallback = on_device_lost;
    // [...] Build device descriptor
    context.device = request_device_sync(context.adapter, &device_desc);

    wgpuDeviceSetUncapturedErrorCallback(context.device, on_device_error, NULL /* pUserData */);
    PRINT_INFO("Got device: %i", context.device);
    //END Device creation

    context.queue = wgpuDeviceGetQueue(context.device);

    PRINT_INFO("WebGPU renderer initialized successfully.");
    return TRUE;
}

void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
    // Destroy in the opposite order of creation.
    PRINT_DEBUG("Destroying WebGPU Queue...");
    wgpuQueueRelease(context.queue);

    PRINT_DEBUG("Destroying WebGPU Device...");
    wgpuDeviceRelease(context.device);

    PRINT_DEBUG("Destroying WebGPU Adapter...");
    wgpuAdapterRelease(context.adapter);

    PRINT_DEBUG("Destroying WebGPU Surface...");
    wgpuSurfaceRelease(context.surface);

    PRINT_DEBUG("Destroying WebGPU instance...");
    // We clean up the WebGPU instance
    wgpuInstanceRelease(context.instance);

}

void webgpu_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height) {
}

b8 webgpu_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    return TRUE;
}

b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    return TRUE;
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
    adapter_data->requestEnded = TRUE;
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
    device_data->requestEnded = TRUE;
};



void on_device_lost(WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
    PRINT_ERROR("Device lost: reason ", reason);
    if (message) PRINT_ERROR("message: ", message);
};

void on_device_error(WGPUErrorType type, char const* message, void* /* pUserData */) {
    PRINT_ERROR("Uncaptured device error: type ", type);
    if (message) PRINT_ERROR("message: ", message);
};