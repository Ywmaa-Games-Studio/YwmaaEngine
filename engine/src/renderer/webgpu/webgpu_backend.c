#include "webgpu_types.inl"

#include "webgpu_backend.h"
#include "webgpu_platform.h"
#include "webgpu_swapchain.h"
#include "webgpu_device.h"
#include "webgpu_image.h"
#include "webgpu_buffer.h"
#include "shaders/webgpu_material_shader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/application.h"

#include "systems/material_system.h"

b8 webgpu_upload_data_range(WEBGPU_CONTEXT* context, WEBGPU_BUFFER* buffer, u64* out_offset, u64 size, const void* data) {
    // Allocate space in the buffer.
    if (!webgpu_buffer_allocate(buffer, size, out_offset)) {
        PRINT_ERROR("upload_data_range failed to allocate from the given buffer!");
        return false;
    }

    webgpu_buffer_load_data(context, buffer, *out_offset, size, data);

    return true;
}

void webgpu_free_data_range(WEBGPU_BUFFER* buffer, u64 offset, u64 size) {
    if (buffer) {
        webgpu_buffer_free(buffer, size, offset);
    }
}

b8 webgpu_create_buffers(WEBGPU_CONTEXT* context);
void webgpu_destroy_buffers(WEBGPU_CONTEXT* context);

b8 wgpu_recreate_swapchain(RENDERER_BACKEND* backend);

WGPUTextureView get_next_surface_texture_view();
WGPUTextureView get_depth_texture_view();
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

    context.depth_view = get_depth_texture_view(context.framebuffer_width, context.framebuffer_height);
    if (!context.depth_view) {
        PRINT_ERROR("Failed to create depth texture view.");
        return false;
    }

    // Create builtin shaders
    if (!webgpu_material_shader_create(&context, &context.material_shader)) {
        PRINT_ERROR("Error loading built-in basic_lighting shader.");
        return false;
    }

    webgpu_create_buffers(&context);

    // Mark all geometries as invalid
    for (u32 i = 0; i < WEBGPU_MAX_GEOMETRY_COUNT; ++i) {
        context.geometries[i].id = INVALID_ID;
    }


    PRINT_INFO("WebGPU renderer initialized successfully.");
    return true;
}

void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
    // Destroy in the opposite order of creation.


    webgpu_destroy_buffers(&context);

    webgpu_material_shader_destroy(&context, &context.material_shader);

    wgpuTextureViewRelease(context.depth_view);

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
    encoder_desc.label = (WGPUStringView){"command encoder", sizeof("command encoder")};
    context.encoder = wgpuDeviceCreateCommandEncoder(context.device, &encoder_desc);

    //START Render Pass
    // Describe Render Pass
    WGPURenderPassColorAttachment render_pass_color_attachment = {};
    // [...] Describe the attachment
    render_pass_color_attachment.nextInChain = NULL;
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

    // We now add a depth/stencil attachment:
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    // Setup depth/stencil attachment

    // The view of the depth texture
    depthStencilAttachment.view = context.depth_view;

    // The initial value of the depth buffer, meaning "far"
    depthStencilAttachment.depthClearValue = 1.0f;
    // Operation settings comparable to the color attachment
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    // we could turn off writing to the depth buffer globally here
    depthStencilAttachment.depthReadOnly = false;

    // Stencil setup, mandatory but unused
    depthStencilAttachment.stencilClearValue = 0;
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.stencilReadOnly = true;
    
    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.nextInChain = NULL;
    //render_pass_desc.label = "";
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &render_pass_color_attachment;
    render_pass_desc.depthStencilAttachment = &depthStencilAttachment;
    render_pass_desc.timestampWrites = NULL;

    context.render_pass = wgpuCommandEncoderBeginRenderPass(context.encoder, &render_pass_desc);
    
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
    cmd_buffer_descriptor.label = (WGPUStringView){"Command buffer", sizeof("Command buffer")};
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

b8 webgpu_renderer_create_geometry(GEOMETRY* geometry, u32 vertex_count, const Vertex3D* vertices, u32 index_count, const u32* indices) {
    if (!vertex_count || !vertices) {
        PRINT_ERROR("webgpu_renderer_create_geometry requires vertex data, and none was supplied. vertex_count=%d, vertices=%p", vertex_count, vertices);
        return false;
    }

    // Check if this is a re-upload. If it is, need to free old data afterward.
    b8 is_reupload = geometry->internal_id != INVALID_ID;
    WEBGPU_GEOMETRY_DATA old_range;

    WEBGPU_GEOMETRY_DATA* internal_data = 0;
    if (is_reupload) {
        internal_data = &context.geometries[geometry->internal_id];

        // Take a copy of the old range.
        old_range.index_buffer_offset = internal_data->index_buffer_offset;
        old_range.index_count = internal_data->index_count;
        old_range.index_size = internal_data->index_size;
        old_range.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_range.vertex_count = internal_data->vertex_count;
        old_range.vertex_size = internal_data->vertex_size;
    } else {
        for (u32 i = 0; i < WEBGPU_MAX_GEOMETRY_COUNT; ++i) {
            if (context.geometries[i].id == INVALID_ID) {
                // Found a free index.
                geometry->internal_id = i;
                context.geometries[i].id = i;
                internal_data = &context.geometries[i];
                break;
            }
        }
    }
    if (!internal_data) {
        PRINT_ERROR("webgpu_renderer_create_geometry failed to find a free index for a new geometry upload. Adjust config to allow for more.");
        return false;
    }

    // Vertex data.
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_size = sizeof(Vertex3D) * vertex_count;
    u32 total_size = vertex_count * internal_data->vertex_size;
    if (!webgpu_upload_data_range(&context, &context.object_vertex_buffer, &internal_data->vertex_buffer_offset, total_size, vertices)) {
        PRINT_ERROR("webgpu_renderer_create_geometry failed to upload to the vertex buffer!");
        return false;
    }

    // Index data, if applicable
    if (index_count && indices) {
        internal_data->index_count = index_count;
        internal_data->index_size = sizeof(u32) * index_count;
        total_size = index_count * internal_data->index_size;
        if (!webgpu_upload_data_range(&context, &context.object_index_buffer, &internal_data->index_buffer_offset, total_size, indices)){
            PRINT_ERROR("webgpu_renderer_create_geometry failed to upload to the index buffer!");
            return false;
        }
    }

    if (internal_data->generation == INVALID_ID) {
        internal_data->generation = 0;
    } else {
        internal_data->generation++;
    }

    if (is_reupload) {
        // Free vertex data
        webgpu_free_data_range(&context.object_vertex_buffer, old_range.vertex_buffer_offset, old_range.vertex_size);

        // Free index data, if applicable
        if (old_range.index_size > 0) {
            webgpu_free_data_range(&context.object_index_buffer, old_range.index_buffer_offset, old_range.index_size);
        }
    }

    return true;
}

void webgpu_renderer_destroy_geometry(GEOMETRY* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        WEBGPU_GEOMETRY_DATA* internal_data = &context.geometries[geometry->internal_id];

        // Free vertex data
        webgpu_free_data_range(&context.object_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_size);

        // Free index data, if applicable
        if (internal_data->index_size > 0) {
            webgpu_free_data_range(&context.object_index_buffer, internal_data->index_buffer_offset, internal_data->index_size);
        }

        // Clean up data.
        yzero_memory(internal_data, sizeof(WEBGPU_GEOMETRY_DATA));
        internal_data->id = INVALID_ID;
        internal_data->generation = INVALID_ID;
    }
}

void webgpu_renderer_draw_geometry(GEOMETRY_RENDER_DATA data) {
    // Ignore non-uploaded geometries.
    if (data.geometry && data.geometry->internal_id == INVALID_ID) {
        return;
    }

    WEBGPU_GEOMETRY_DATA* buffer_data = &context.geometries[data.geometry->internal_id];
    
    webgpu_material_shader_use(&context, &context.material_shader);
    
    webgpu_material_shader_set_model(&context, &context.material_shader, data.model);
    
    MATERIAL* m = 0;
    if (data.geometry->material) {
        m = data.geometry->material;
    } else {
        m = material_system_get_default();
    }
    webgpu_material_shader_apply_material(&context, &context.material_shader, m);
    
    // Set vertex buffer while encoding the render pass
    wgpuRenderPassEncoderSetVertexBuffer(context.render_pass, 0, context.object_vertex_buffer.handle, buffer_data->vertex_buffer_offset, wgpuBufferGetSize(context.object_vertex_buffer.handle));
    
    // Draw indexed or non-indexed.
    if (buffer_data->index_count > 0) {
        // Bind index buffer at offset.
        wgpuRenderPassEncoderSetIndexBuffer(context.render_pass, context.object_index_buffer.handle, WGPUIndexFormat_Uint32, buffer_data->index_buffer_offset, wgpuBufferGetSize(context.object_index_buffer.handle));
        // Issue the draw.
        wgpuRenderPassEncoderDrawIndexed(context.render_pass, buffer_data->index_count, 1, 0, 0, 0);
    } else {
        wgpuRenderPassEncoderDraw(context.render_pass, buffer_data->vertex_count, 1, 0, 0);
    }
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
    context.depth_view = get_depth_texture_view(cached_framebuffer_width, cached_framebuffer_height);
    if (!context.depth_view) {
        PRINT_ERROR("Failed to create depth texture view.");
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

WGPUTextureView get_depth_texture_view(u32 width, u32 height) {
    // Create the depth texture
    WGPUTextureFormat depthTextureFormat = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor depthTextureDesc;
    depthTextureDesc.label = (WGPUStringView){"Depth Texture", sizeof("Depth Texture")};
    depthTextureDesc.nextInChain = NULL;
    depthTextureDesc.dimension = WGPUTextureDimension_2D;
    depthTextureDesc.format = depthTextureFormat;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = 1;
    depthTextureDesc.size.width = width;
    depthTextureDesc.size.height = height;
    depthTextureDesc.size.depthOrArrayLayers = 1;
    depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthTextureDesc.viewFormatCount = 1;
    depthTextureDesc.viewFormats = &depthTextureFormat;
    WGPUTexture depthTexture = wgpuDeviceCreateTexture(context.device, &depthTextureDesc);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depthTextureViewDesc;
    depthTextureViewDesc.nextInChain = NULL;
    depthTextureViewDesc.label = (WGPUStringView){"Depth Texture View", sizeof("Depth Texture View")};
    depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.format = depthTextureFormat;
    WGPUTextureView depthTextureView = wgpuTextureCreateView(depthTexture, &depthTextureViewDesc);
    return depthTextureView;

    // Destroy the depth texture and its view
    wgpuTextureDestroy(depthTexture);
    wgpuTextureRelease(depthTexture);
}

WGPUTextureView get_next_surface_texture_view() {
    //Get the next surface texture
    WGPUSurfaceTexture surface_texture;
    surface_texture.nextInChain = NULL;
    wgpuSurfaceGetCurrentTexture(context.surface, &surface_texture);

    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
        return NULL;
    }

    //Create surface texture view
    WGPUTextureViewDescriptor view_descriptor;
    view_descriptor.nextInChain = NULL;
    view_descriptor.label = (WGPUStringView){"Surface texture view", sizeof("Surface texture view")};
    view_descriptor.format = wgpuTextureGetFormat(surface_texture.texture);
    view_descriptor.dimension = WGPUTextureViewDimension_2D;
    view_descriptor.baseMipLevel = 0;
    view_descriptor.mipLevelCount = 1;
    view_descriptor.baseArrayLayer = 0;
    view_descriptor.arrayLayerCount = 1;
    view_descriptor.aspect = WGPUTextureAspect_All;
    view_descriptor.usage = WGPUTextureUsage_RenderAttachment;
    WGPUTextureView target_view = wgpuTextureCreateView(surface_texture.texture, &view_descriptor);
    return target_view;
}


b8 webgpu_create_buffers(WEBGPU_CONTEXT* context) {
    const u64 vertex_buffer_size = sizeof(Vertex3D) * 1024 * 1024;
    if (!webgpu_buffer_create(context, vertex_buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, false, &context->object_vertex_buffer)) {
        PRINT_ERROR("Error creating vertex buffer.");
        return false;
    }

    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!webgpu_buffer_create(context, index_buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index, false, &context->object_index_buffer)) {
        PRINT_ERROR("Error creating index buffer.");
        return false;
    }

    
    return true;
}

void webgpu_destroy_buffers(WEBGPU_CONTEXT* context){
    webgpu_buffer_destroy(&context->object_vertex_buffer);
    webgpu_buffer_destroy(&context->object_index_buffer);

}



void webgpu_renderer_create_texture(const u8* pixels, TEXTURE* texture){
    // Internal data creation.
    // TODO: Use an allocator for this.
    texture->internal_data = (WEBGPU_TEXTURE_DATA*)yallocate_aligned(sizeof(WEBGPU_TEXTURE_DATA), 8, MEMORY_TAG_TEXTURE);
    WEBGPU_TEXTURE_DATA* data = (WEBGPU_TEXTURE_DATA*)texture->internal_data;

    webgpu_image_create(
        &context,
        WGPUTextureDimension_2D,
        texture->width,
        texture->height,
        WGPUTextureFormat_RGBA8Unorm,
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        true,
        WGPUTextureAspect_All,
        &data->image);

    // Copy the data from the buffer.
    webgpu_image_copy_from_buffer(&context, &data->image, pixels, texture->channel_count);

    // Create a sampler
    WGPUSamplerDescriptor sampler_desc;
    // TODO: These filters should be configurable.
    sampler_desc.nextInChain = NULL;
    sampler_desc.label = (WGPUStringView){"Texture sampler", sizeof("Texture sampler")};
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

    texture->generation++;
}

void webgpu_renderer_destroy_texture(TEXTURE* texture){
    WEBGPU_TEXTURE_DATA* data = (WEBGPU_TEXTURE_DATA*)texture->internal_data;


    if (data) {
        webgpu_image_destroy(&data->image);
        yzero_memory(&data->image, sizeof(WEBGPU_IMAGE));
        wgpuSamplerRelease(data->sampler);
        data->sampler = 0;

        yfree_aligned(texture->internal_data, sizeof(WEBGPU_TEXTURE_DATA), 8, MEMORY_TAG_TEXTURE);
    }
    

    yzero_memory(texture, sizeof(struct TEXTURE));
    
}

b8 webgpu_renderer_create_material(struct MATERIAL* material) {
    if (material) {
        if (!webgpu_material_shader_acquire_resources(&context, &context.material_shader, material)) {
            PRINT_ERROR("webgpu_renderer_create_material - Failed to acquire shader resources.");
            return false;
        }

        PRINT_DEBUG("Renderer: Material created.");
        return true;
    }

    PRINT_ERROR("webgpu_renderer_create_material called with nullptr. Creation failed.");
    return false;
}

void webgpu_renderer_destroy_material(struct MATERIAL* material) {
    if (material) {
        if (material->internal_id != INVALID_ID) {
            webgpu_material_shader_release_resources(&context, &context.material_shader, material);
        } else {
            PRINT_WARNING("webgpu_renderer_destroy_material called with internal_id=INVALID_ID. Nothing was done.");
        }
    } else {
        PRINT_WARNING("webgpu_renderer_destroy_material called with nullptr. Nothing was done.");
    }
}