#include "webgpu_backend.h"
#include "webgpu_platform.h"

#include "webgpu_types.inl"
#include "core/logger.h"
#include "core/ystring.h"
#include "core/ymemory.h"
#include "core/asserts.h"
#include "core/application.h"

#include "io/filesystem.h"

#include "variants/darray.h"

#include "platform/platform.h"

b8 webgpu_pipeline_create();
void webgpu_pipeline_destroy();

b8 webgpu_device_create();
void webgpu_device_destroy();

WGPUAdapter request_adapter_sync(WGPUInstance instance, WGPURequestAdapterOptions const * options);
void on_adapter_request_ended(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData);

WGPUDevice request_device_sync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);
void on_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData);
void on_device_lost(WGPUDeviceLostReason reason, char const* message, void* /* pUserData */);
void on_device_error(WGPUErrorType type, char const* message, void* /* pUserData */);

void on_queue_work_done(WGPUQueueWorkDoneStatus status, void* /* pUserData */);

b8 webgpu_swapchain_create(
    u32 width,
    u32 height);
void webgpu_swapchain_destroy();
b8 webgpu_recreate_swapchain(RENDERER_BACKEND* backend);

WGPUTextureView get_next_surface_texture_view();
// static WebGPU context
static WEBGPU_CONTEXT context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;


b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const char* application_name) {
    application_get_framebuffer_size(&cached_framebuffer_width, &cached_framebuffer_height);
    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    if (!webgpu_device_create()){
        return false;
    }

    if (!webgpu_swapchain_create(context.framebuffer_width, context.framebuffer_height)){
        return false;
    }

    webgpu_pipeline_create();

    PRINT_INFO("WebGPU renderer initialized successfully.");
    return true;
}

void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
    // Destroy in the opposite order of creation.
    // We no longer need to access the shader module
    
    webgpu_pipeline_destroy();

    webgpu_swapchain_destroy();

    webgpu_device_destroy();

}

void webgpu_renderer_backend_on_resized(RENDERER_BACKEND* backend, u16 width, u16 height) {
    // Update the "framebuffer size generation", a counter which indicates when the
    // framebuffer size has been updated.
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;
    context.framebuffer_size_generation++;

    PRINT_INFO("WebGPU renderer backend->resized: w/h/gen: %i/%i/%llu", width, height, context.framebuffer_size_generation);
}

b8 webgpu_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    // Check if recreating swap chain and boot out.
    if (context.recreating_swapchain) {
        PRINT_INFO("Recreating swapchain, booting.");
        return false;
    }

    // Check if the framebuffer has been resized. If so, a new swapchain must be created.
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {
        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
        if (!webgpu_recreate_swapchain(backend)) {
            return false;
        }

        PRINT_INFO("Resized, booting.");
        return false;
    }

    //Create Texture
    context.target_view = get_next_surface_texture_view();
    if (!context.target_view) return false;

    WGPUCommandEncoderDescriptor encoder_desc = {};
    encoder_desc.nextInChain = NULL;
    encoder_desc.label = "command encoder";
    context.encoder = wgpuDeviceCreateCommandEncoder(context.device, &encoder_desc);

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
    color.r = 0.0;
    color.g = 0.2;
    color.b = 0.2;
    color.a = 1.0;
    render_pass_color_attachment.clearValue = color;

    context.render_pass = wgpuCommandEncoderBeginRenderPass(context.encoder, &render_pass_desc);
    // [...] Use Render Pass

        // Select which render pipeline to use
    wgpuRenderPassEncoderSetPipeline(context.render_pass, context.pipeline);
    // Draw 1 instance of a 3-vertices shape
    wgpuRenderPassEncoderDraw(context.render_pass, 3, 1, 0, 0);

    return true;
}
void webgpu_renderer_update_global_state(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode) {

}
b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time) {

    wgpuRenderPassEncoderEnd(context.render_pass);
    wgpuRenderPassEncoderRelease(context.render_pass);
    //END Render Pass

    WGPUCommandBufferDescriptor cmd_buffer_descriptor = {};
    cmd_buffer_descriptor.nextInChain = NULL;
    cmd_buffer_descriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(context.encoder, &cmd_buffer_descriptor);
    wgpuCommandEncoderRelease(context.encoder);

    // Finally submit the command queue
    wgpuQueueSubmit(context.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    wgpuTextureViewRelease(context.target_view);

    //Present Texture
    wgpuSurfacePresent(context.surface);

    return true;
}

b8 webgpu_pipeline_create(){
    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.hintCount = 0;
    shaderDesc.hints = NULL;

    WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = NULL;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    // Build file name.
    char file_name[512] = "assets/shaders/shader.wgsl";

    // Obtain file handle.
    FILE_HANDLE handle;
    if (!filesystem_open(file_name, FILE_MODE_READ, false, &handle)) {
        PRINT_ERROR("Unable to read wgsl shader: %s.", file_name);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&handle, &file_size)) {
        PRINT_ERROR("Unable to wgsl shader file: %s.", file_name);
        filesystem_close(&handle);
        return false;
    }

    // Read the entire file as binary.
    u64 size = 0;
    u8 extra_size = 1; // This is required to add extra size for empty space
    const char* code = yallocate(sizeof(char) * file_size +extra_size, MEMORY_TAG_STRING);
    if (!filesystem_read_all_text(&handle, code, &size)) {
        PRINT_ERROR("Unable to text read shader module: %s.", file_name);
        return false;
    }

    // Close the file.
    filesystem_close(&handle);

    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.code = code;
    PRINT_INFO("Got shader module: ");
    context.shaderModule = wgpuDeviceCreateShaderModule(context.device, &shaderDesc);
    PRINT_INFO("Got shader module: %i", &context.shaderModule);


    // [...] Describe render pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.nextInChain = NULL;
// [...] Describe vertex pipeline state
    // We do not use any vertex buffer for this first simplistic example
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = NULL;
    // NB: We define the 'shaderModule' in the second part of this chapter.
    // Here we tell that the programmable vertex shader stage is described
    // by the function called 'vs_main' in that module.
    pipelineDesc.vertex.module = context.shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = NULL;
// [...] Describe primitive pipeline state
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;

    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;

    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;
// [...] Describe fragment pipeline state
    // We tell that the programmable fragment shader stage is described
    // by the function called 'fs_main' in the shader module.
    WGPUFragmentState fragmentState = {};
    fragmentState.module = context.shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = NULL;
// [...] We'll configure the blending stage here
    WGPUBlendState blendState = {};
// [...] Configure color blending equation
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
// [...] Configure alpha blending equation
    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;
    WGPUColorTargetState colorTarget = {};
    colorTarget.format = context.swapchain_format;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
// [...] Describe stencil/depth pipeline state
    // We do not use stencil/depth testing for now
    pipelineDesc.depthStencil = NULL;
// [...] Describe multi-sampling state
    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;
// [...] Describe pipeline layout
    pipelineDesc.layout = NULL;
    context.pipeline = wgpuDeviceCreateRenderPipeline(context.device, &pipelineDesc);
    wgpuShaderModuleRelease(context.shaderModule);
    return true;
}

void webgpu_pipeline_destroy()
{
    
}

b8 webgpu_device_create(){
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
        return false;
    }

    //START Surface
    PRINT_DEBUG("Creating WebGPU surface...");
    if (!platform_create_webgpu_surface(&context)) {
        PRINT_ERROR("Failed to create platform surface!");
        return false;
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

	WGPUSupportedLimits supported_limits;
	wgpuAdapterGetLimits(context.adapter, &supported_limits);

#ifdef _DEBUG
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
#endif

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
    context.swapchain_format = wgpuSurfaceGetPreferredFormat(context.surface, context.adapter);
    //END Device creation

    context.queue = wgpuDeviceGetQueue(context.device);

    PRINT_INFO("Queues obtained.");
    wgpuQueueOnSubmittedWorkDone(context.queue, on_queue_work_done, NULL /* pUserData */);

    PRINT_DEBUG("Destroying WebGPU Adapter...");
    wgpuAdapterRelease(context.adapter);
    
    return context.device != NULL;
}
void webgpu_device_destroy(){
    PRINT_DEBUG("Destroying WebGPU Queue...");
    wgpuQueueRelease(context.queue);

    PRINT_DEBUG("Destroying WebGPU Device...");
    wgpuDeviceRelease(context.device);

    PRINT_DEBUG("Destroying WebGPU Surface...");
    wgpuSurfaceRelease(context.surface);

    PRINT_DEBUG("Destroying WebGPU instance...");
    // We clean up the WebGPU instance
    wgpuInstanceRelease(context.instance);
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



b8 webgpu_swapchain_create(
    u32 width,
    u32 height) {
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = NULL;
    config.width = width;
    config.height = height;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.format = context.swapchain_format;
    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = NULL;
    config.device = context.device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(context.surface, &config);
    

    return context.surface != NULL;
}

void webgpu_swapchain_destroy(){
    wgpuSurfaceUnconfigure(context.surface);
}

b8 webgpu_recreate_swapchain(RENDERER_BACKEND* backend) {
    // If already being recreated, do not try again.
    if (context.recreating_swapchain) {
        PRINT_DEBUG("recreate_swapchain called when already recreating. Booting.");
        return false;
    }

    // Detect if the window is too small to be drawn to
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
        PRINT_DEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
        return false;
    }

    // Mark as recreating if the dimensions are valid.
    context.recreating_swapchain = true;

    webgpu_swapchain_destroy();
    
    // Re-init
    // Swapchain
    if (!webgpu_swapchain_create(cached_framebuffer_width, cached_framebuffer_height)){
        PRINT_DEBUG("Failed to recreate swapchain");
        return false;
    }   


    // Sync the framebuffer size with the cached sizes.
    context.framebuffer_width = cached_framebuffer_width;
    context.framebuffer_height = cached_framebuffer_height;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;


    // Update framebuffer size generation.
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    // Clear the recreating flag.
    context.recreating_swapchain = false;

    return true;
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