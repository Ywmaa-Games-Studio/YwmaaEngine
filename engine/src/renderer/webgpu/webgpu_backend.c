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

void on_queue_work_done(WGPUQueueWorkDoneStatus status, void* /* pUserData */);

void webgpu_swapchain_create(
    WEBGPU_CONTEXT* context,
    u32 width,
    u32 height);
WGPUTextureView get_next_surface_texture_view();
// static WebGPU context
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

    PRINT_INFO("Queues obtained.");
    wgpuQueueOnSubmittedWorkDone(context.queue, on_queue_work_done, NULL /* pUserData */);


    // Swapchain
    //TODO: Remove the hardcoded w & h sizes
    context.framebuffer_width = 640;
    context.framebuffer_height = 480;
    webgpu_swapchain_create(
        &context,
        context.framebuffer_width,
        context.framebuffer_height);

    WGPUCommandEncoderDescriptor encoder_desc = {};
    encoder_desc.nextInChain = NULL;
    encoder_desc.label = "command encoder";
    context.encoder = wgpuDeviceCreateCommandEncoder(context.device, &encoder_desc);

    PRINT_INFO("WebGPU renderer initialized successfully.");
    return TRUE;
}

void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
    // Destroy in the opposite order of creation.
    wgpuCommandEncoderRelease(context.encoder);

    wgpuSurfaceUnconfigure(context.surface);

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
    //Create Texture
    context.target_view = get_next_surface_texture_view();
    if (!context.target_view) return FALSE;

    //START Render Pass
    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.nextInChain = NULL;

    // Describe Render Pass

    WGPURenderPassColorAttachment render_pass_color_attachment = {};

    // [...] Describe the attachment

    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &render_pass_color_attachment;
    render_pass_desc.depthStencilAttachment = NULL;
    render_pass_desc.timestampWrites = NULL;

    render_pass_color_attachment.view = context.target_view;

    render_pass_color_attachment.resolveTarget = NULL;

    render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
    render_pass_color_attachment.storeOp = WGPUStoreOp_Store;

    WGPUColor color = {};
    color.r = 0.9;
    color.g = 0.1;
    color.b = 0.2;
    color.a = 1.0;
    render_pass_color_attachment.clearValue = color;

    context.render_pass = wgpuCommandEncoderBeginRenderPass(context.encoder, &render_pass_desc);
    // [...] Use Render Pass

    return TRUE;
}

b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time) {

    wgpuRenderPassEncoderEnd(context.render_pass);
    wgpuRenderPassEncoderRelease(context.render_pass);
    //END Render Pass


    WGPUCommandBufferDescriptor cmd_buffer_descriptor = {};
    cmd_buffer_descriptor.nextInChain = NULL;
    cmd_buffer_descriptor.label = "Render Pass buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(context.encoder, &cmd_buffer_descriptor);
    

    // Finally submit the command queue
    PRINT_DEBUG("Submitting command...");
    wgpuQueueSubmit(context.queue, 1, &command);
    wgpuCommandBufferRelease(command);
    PRINT_DEBUG("Command submitted.");

    //for (int i = 0 ; i < 5 ; ++i) {
    //    PRINT_DEBUG("Tick/Poll device...");
    //    wgpuDevicePoll(context.device, FALSE, NULL);
    //}

    //Present Texture
    wgpuSurfacePresent(context.surface);

    
    wgpuTextureViewRelease(context.target_view);
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



void on_queue_work_done(WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
    PRINT_DEBUG("Queued work finished with status: ", status);
};



void webgpu_swapchain_create(
    WEBGPU_CONTEXT* context,
    u32 width,
    u32 height) {
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = NULL;
    config.width = width;
    config.height = height;
    config.usage = WGPUTextureUsage_RenderAttachment;
    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(context->surface, context->adapter);
    config.format = surfaceFormat;
    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = NULL;
    config.device = context->device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(context->surface, &config);
}

WGPUTextureView get_next_surface_texture_view() {
    //Get the next surface texture
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(context.surface, &surface_texture);

    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return NULL;
    }

    //Create surface texture view
    WGPUTextureViewDescriptor view_descriptor;
    view_descriptor.nextInChain = NULL;
    view_descriptor.label = "Surface texture view";
    view_descriptor.format = wgpuTextureGetFormat(surface_texture.texture);
    view_descriptor.dimension = WGPUTextureViewDimension_2D;
    view_descriptor.baseMipLevel = 0;
    view_descriptor.mipLevelCount = 1;
    view_descriptor.baseArrayLayer = 0;
    view_descriptor.arrayLayerCount = 1;
    view_descriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView target_view = wgpuTextureCreateView(surface_texture.texture, &view_descriptor);
    return target_view;
}