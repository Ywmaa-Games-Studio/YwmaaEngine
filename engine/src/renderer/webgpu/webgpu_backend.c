#include "webgpu_types.inl"

#include "webgpu_backend.h"
#include "webgpu_platform.h"
#include "webgpu_swapchain.h"
#include "webgpu_device.h"
#include "webgpu_image.h"
#include "shaders/webgpu_material_shader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/application.h"

b8 webgpu_create_buffers();
void webgpu_destroy_buffers();

b8 wgpu_recreate_swapchain(RENDERER_BACKEND* backend);

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

    // We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = NULL;

// We create the instance using this descriptor
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    PRINT_INFO("Detected Web Build Use EMSCRIPTEN.");
    context->instance = wgpuCreateInstance(nullptr);
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

    if (!webgpu_device_create(&context)){
        return false;
    }

    if (!webgpu_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height)){
        return false;
    }

    // Create builtin shaders
    if (!webgpu_material_shader_create(&context, &context.material_shader)) {
        PRINT_ERROR("Error loading built-in basic_lighting shader.");
        return false;
    }

    webgpu_create_buffers();

    u32 object_id = 0;
    if (!webgpu_material_shader_acquire_resources(&context, &context.material_shader, &object_id)) {
        PRINT_ERROR("Failed to acquire shader resources.");
        return false;
    }

    PRINT_INFO("WebGPU renderer initialized successfully.");
    return true;
}

void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
    // Destroy in the opposite order of creation.


    webgpu_destroy_buffers();

    webgpu_material_shader_destroy(&context, &context.material_shader);

    webgpu_swapchain_destroy(&context);

    webgpu_device_destroy(&context);

    PRINT_DEBUG("Destroying WebGPU Surface...");
    wgpuSurfaceRelease(context.surface);

    PRINT_DEBUG("Destroying WebGPU instance...");
    // We clean up the WebGPU instance
    wgpuInstanceRelease(context.instance);

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
    context.frame_delta_time = delta_time;
    // Check if recreating swap chain and boot out.
    if (context.recreating_swapchain) {
        PRINT_INFO("Recreating swapchain, booting.");
        return false;
    }

    // Check if the framebuffer has been resized. If so, a new swapchain must be created.
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {
        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
        if (!wgpu_recreate_swapchain(backend)) {
            return false;
        }

        // Sync the framebuffer size with the cached sizes.
        context.framebuffer_width = cached_framebuffer_width;
        context.framebuffer_height = cached_framebuffer_height;
        cached_framebuffer_width = 0;
        cached_framebuffer_height = 0;

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
    wgpuRenderPassEncoderSetPipeline(context.render_pass, context.material_shader.pipeline.handle);

    return true;
}
void webgpu_renderer_update_global_state(Matrice4 projection, Matrice4 view, Vector3 view_position, Vector4 ambient_colour, i32 mode) {

    webgpu_material_shader_use(&context, &context.material_shader);

    context.material_shader.global_ubo.projection = projection;
    context.material_shader.global_ubo.view = view;

    webgpu_material_shader_update_global_state(&context, &context.material_shader, context.frame_delta_time);
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

void webgpu_backend_update_object(GEOMETRY_RENDER_DATA data) {
    webgpu_material_shader_update_object(&context, &context.material_shader, data);

    webgpu_material_shader_use(&context, &context.material_shader);

    // Set vertex buffer while encoding the render pass
    wgpuRenderPassEncoderSetVertexBuffer(context.render_pass, 0, context.object_vertex_buffer, 0, wgpuBufferGetSize(context.object_vertex_buffer));
    wgpuRenderPassEncoderSetIndexBuffer(context.render_pass, context.object_index_buffer, WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(context.object_index_buffer));

    wgpuRenderPassEncoderDrawIndexed(context.render_pass, 6, 1, 0, 0, 0);
}


b8 wgpu_recreate_swapchain(RENDERER_BACKEND* backend) {
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

    webgpu_recreate_swapchain(&context, backend, cached_framebuffer_width, cached_framebuffer_height);

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


b8 webgpu_create_buffers() {
    
    // TODO: temporary test code
    const u32 vert_count = 4;
    Vertex3D verts[vert_count];
    yzero_memory(verts, sizeof(Vertex3D) * vert_count);
    
    const f32 f = 10.0;

    verts[0].position.x = -0.5 * f;
    verts[0].position.y = -0.5 * f;
    verts[0].texcoord.x = 0.0;
    verts[0].texcoord.y = 0.0;

    verts[1].position.x = 0.5 * f;
    verts[1].position.y = 0.5 * f;
    verts[1].texcoord.x = 1.0;
    verts[1].texcoord.y = 1.0;

    verts[2].position.x = -0.5 * f;
    verts[2].position.y = 0.5 * f;
    verts[2].texcoord.x = 0.0;
    verts[2].texcoord.y = 1.0;

    verts[3].position.x = 0.5 * f;
    verts[3].position.y = -0.5 * f;
    verts[3].texcoord.x = 1.0;
    verts[3].texcoord.y = 0.0;

    const u32 index_count = 6;
    u32 indices[index_count] = {0, 1, 2, 0, 3, 1};
    // TODO: end temp code

    WGPUBufferDescriptor vertex_buffer_desc = {};
    vertex_buffer_desc.nextInChain = NULL;
    vertex_buffer_desc.label = "Vertex Buffer";
    vertex_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    vertex_buffer_desc.size = sizeof(Vertex3D) * vert_count;
    vertex_buffer_desc.mappedAtCreation = false;
    context.object_vertex_buffer = wgpuDeviceCreateBuffer(context.device, &vertex_buffer_desc);

    WGPUBufferDescriptor index_buffer_desc = {};
    index_buffer_desc.nextInChain = NULL;
    index_buffer_desc.label = "Index Buffer";
    index_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    index_buffer_desc.size = sizeof(u32) * index_count;
    index_buffer_desc.mappedAtCreation = false;
    context.object_index_buffer = wgpuDeviceCreateBuffer(context.device, &index_buffer_desc);

    // Upload geometry data to the buffer
    wgpuQueueWriteBuffer(context.queue, context.object_vertex_buffer, 0, verts, vertex_buffer_desc.size);
    wgpuQueueWriteBuffer(context.queue, context.object_index_buffer, 0, indices, index_buffer_desc.size);

    
    return true;
}

void webgpu_destroy_buffers(){
    wgpuBufferDestroy(context.object_vertex_buffer);
    wgpuBufferDestroy(context.object_index_buffer);
    wgpuBufferRelease(context.object_vertex_buffer);
    wgpuBufferRelease(context.object_index_buffer);

}



void webgpu_renderer_create_texture(const char* name, i32 width, i32 height, i32 channel_count, const u8* pixels, b8 has_transparency, struct TEXTURE* out_texture){
    out_texture->width = width;
    out_texture->height = height;
    out_texture->channel_count = channel_count;
    out_texture->generation = INVALID_ID;
    // Internal data creation.
    // TODO: Use an allocator for this.
    out_texture->internal_data = (WEBGPU_TEXTURE_DATA*)yallocate_aligned(sizeof(WEBGPU_TEXTURE_DATA), 4, MEMORY_TAG_TEXTURE);
    WEBGPU_TEXTURE_DATA* data = (WEBGPU_TEXTURE_DATA*)out_texture->internal_data;

    webgpu_image_create(
        &context,
        WGPUTextureDimension_2D,
        width,
        height,
        WGPUTextureFormat_RGBA8Unorm,
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        true,
        WGPUTextureAspect_All,
        &data->image);

    // Copy the data from the buffer.
    webgpu_image_copy_from_buffer(&context, &data->image, pixels, channel_count);

    // Create a sampler
    WGPUSamplerDescriptor sampler_desc;
    // TODO: These filters should be configurable.
    sampler_desc.magFilter = WGPUFilterMode_Linear;
    sampler_desc.minFilter = WGPUFilterMode_Linear;
    sampler_desc.addressModeU = WGPUAddressMode_Repeat;
    sampler_desc.addressModeV = WGPUAddressMode_Repeat;
    sampler_desc.addressModeW = WGPUAddressMode_Repeat;
    sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    sampler_desc.lodMinClamp = 0.0f;
    sampler_desc.lodMaxClamp = 1.0f;
    sampler_desc.compare = WGPUCompareFunction_Undefined;
    sampler_desc.maxAnisotropy = 16;
    data->sampler = wgpuDeviceCreateSampler(context.device, &sampler_desc);


    out_texture->has_transparency = has_transparency;
    out_texture->generation++;
}

void webgpu_renderer_destroy_texture(TEXTURE* texture){
    WEBGPU_TEXTURE_DATA* data = (WEBGPU_TEXTURE_DATA*)texture->internal_data;


    if (data) {
        webgpu_image_destroy(&data->image);
        yzero_memory(&data->image, sizeof(WEBGPU_IMAGE));
        wgpuSamplerRelease(data->sampler);
        data->sampler = 0;

        yfree(texture->internal_data, sizeof(WEBGPU_TEXTURE_DATA), MEMORY_TAG_TEXTURE);
    }
    

    yzero_memory(texture, sizeof(struct TEXTURE));
    
}
