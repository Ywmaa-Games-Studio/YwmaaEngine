#pragma clang optimize off // Disable optimizations here because sometimes they cause damage by removing important zeroed variables

#include "webgpu_types.inl"

#include "webgpu_backend.h"
#include "webgpu_platform.h"
#include "webgpu_swapchain.h"
#include "webgpu_device.h"
#include "webgpu_image.h"
#include "webgpu_shader_utils.h"
#include "webgpu_pipeline.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "core/application.h"
#include "core/event.h"
#include "core/asserts.h"

#include "renderer/renderer_frontend.h"

#include "data_structures/darray.h"

#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/texture_system.h"
#include "systems/resource_system.h"

void webgpu_bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);


b8 wgpu_recreate_swapchain(RENDERER_BACKEND* backend);

WGPUTextureView get_next_surface_texture_view(void);
void get_depth_texture(WGPUTexture* out_depth_texture, WGPUTextureView* out_depth_texture_view, u32 width, u32 height);
// static WebGPU context
static WEBGPU_CONTEXT context = {0};


b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const  RENDERER_BACKEND_CONFIG* config, u8* out_window_render_target_count) {
    // Just set some default values for the framebuffer for now.
    // It doesn't really matter what these are because they will be
    // overridden, but are needed for swapchain creation.
    context.framebuffer_width = 1280;
    context.framebuffer_height = 720;

    // We create a descriptor
    WGPUInstanceDescriptor desc = {0};
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

    // TODO: implement multi-threading.
    context.multithreading_enabled = false;

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

    if (!wgpu_recreate_swapchain(backend)) {
        PRINT_ERROR("Failed to create swapchain.");
        return false;
    }

    // Save off the number of images we have as the number of render targets needed.
    *out_window_render_target_count = 1;

    // Geometry vertex buffer
    const u64 vertex_buffer_size = sizeof(Vertex3D) * 2 * 1024 * 1024;
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_VERTEX, vertex_buffer_size, true, &context.object_vertex_buffer)) {
        PRINT_ERROR("Error creating vertex buffer.");
        return false;
    }

    // Geometry index buffer
    const u64 index_buffer_size = sizeof(u32) * 2 * 1024 * 1024;
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_INDEX, index_buffer_size, true, &context.object_index_buffer)) {
        PRINT_ERROR("Error creating index buffer.");
        return false;
    }

    // Mark all geometries as invalid
    for (u32 i = 0; i < WEBGPU_MAX_GEOMETRY_COUNT; ++i) {
        context.geometries[i].id = INVALID_ID;
    }


    PRINT_INFO("WebGPU renderer initialized successfully.");
    return true;
}

void webgpu_renderer_backend_shutdown(RENDERER_BACKEND* backend) {
    // Destroy in the opposite order of creation.

    renderer_renderbuffer_destroy(&context.object_vertex_buffer);
    renderer_renderbuffer_destroy(&context.object_index_buffer);

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
    context.framebuffer_width = width;
    context.framebuffer_height = height;
    context.framebuffer_size_generation++;

    PRINT_INFO("WebGPU renderer backend->resized: w/h/gen: %i/%i/%llu", width, height, context.framebuffer_size_generation);
}

b8 webgpu_renderer_backend_begin_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    context.frame_delta_time = delta_time;
    // Check if recreating swap chain and boot out.
    if (context.recreating_swapchain) {
        b8 result = wgpuDevicePoll(context.device, true, NULL);
        if (!result) {
            PRINT_ERROR("webgpu_renderer_backend_begin_frame wgpuDevicePoll failed");
            return false;
        }
        PRINT_INFO("Recreating swapchain, booting.");
        return false;
    }

    // Check if the framebuffer has been resized. If so, a new swapchain must be created.
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {
        b8 result = wgpuDevicePoll(context.device, true, NULL);
        if (!result) {
            PRINT_ERROR("webgpu_renderer_backend_begin_frame wgpuDevicePoll failed");
            return false;
        }
        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
        if (!wgpu_recreate_swapchain(backend)) {
            return false;
        }

        PRINT_INFO("Resized, booting.");
        return false;
    }
    // Render Texture View
    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)context.render_texture->internal_data;
    image->handle = NULL;
    image->width = context.framebuffer_width;
    image->height = context.framebuffer_height;
    image->view = get_next_surface_texture_view();
    if (!image->view) {
        PRINT_ERROR("Failed to create surface/render texture view.");
        return false;
    }

    WGPUCommandEncoderDescriptor encoder_desc = {0};
    encoder_desc.nextInChain = NULL;
    encoder_desc.label = (WGPUStringView){"command encoder", sizeof("command encoder")};
    context.encoder = wgpuDeviceCreateCommandEncoder(context.device, &encoder_desc);
    
    return true;
}

b8 webgpu_renderer_backend_end_frame(RENDERER_BACKEND* backend, f32 delta_time) {
    
    WGPUCommandBufferDescriptor cmd_buffer_descriptor = {0};
    cmd_buffer_descriptor.nextInChain = NULL;
    cmd_buffer_descriptor.label = (WGPUStringView){"Command buffer", sizeof("Command buffer")};
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(context.encoder, &cmd_buffer_descriptor);
    wgpuCommandEncoderRelease(context.encoder);
    
    // Finally submit the command queue
    wgpuQueueSubmit(context.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    //Present Texture
    wgpuSurfacePresent(context.surface);

    return true;
}

void webgpu_renderer_viewport_set(Vector4 rect) {
    
}

void webgpu_renderer_viewport_reset(void) {
    
}

void webgpu_renderer_scissor_set(Vector4 rect) {
    
}

void webgpu_renderer_scissor_reset(void) {
    
}

b8 webgpu_renderer_renderpass_begin(RENDERPASS* pass, RENDER_TARGET* target) {
    //START Render Pass

    WEBGPU_RENDERPASS* internal_data = (WEBGPU_RENDERPASS*)pass->internal_data;

    // Max number of attachments
    WGPUTextureView attachment_views[32];
    for (u32 i = 0; i < target->attachment_count; ++i) {
        attachment_views[i] = ((WEBGPU_IMAGE*)target->attachments[i].texture->internal_data)->view;
    }

    for (u32 i = 0; i < target->attachment_count; ++i) {
        if (target->attachments[i].type == RENDER_TARGET_ATTACHMENT_TYPE_COLOR) {
            WGPURenderPassColorAttachment* color_attachment = (WGPURenderPassColorAttachment*)(internal_data->descriptor.colorAttachments);
            color_attachment[i].view = attachment_views[i];
        }
        
    }

    
    b8 do_clear_depth = (pass->clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (do_clear_depth) {
        WGPURenderPassDepthStencilAttachment* depth_stencil_attachment = (WGPURenderPassDepthStencilAttachment*)(internal_data->descriptor.depthStencilAttachment);
        
        for (u32 i = 0; i < target->attachment_count; ++i) {
            if (target->attachments[i].type == RENDER_TARGET_ATTACHMENT_TYPE_DEPTH) {
                if (depth_stencil_attachment) {
                    // If the depth stencil attachment is not set, we need to create it.
                    if (!depth_stencil_attachment->view) {
                        depth_stencil_attachment->view = attachment_views[i];
                        // depth attachment is only one anyway, so we can break.
                        break;
                    }
                }
            }
        }

    }
    
    internal_data->handle = wgpuCommandEncoderBeginRenderPass(context.encoder, &internal_data->descriptor);

    return true;
}

b8 webgpu_renderer_renderpass_end(RENDERPASS* pass) {
    WEBGPU_RENDERPASS* internal_data = (WEBGPU_RENDERPASS*)pass->internal_data;
    wgpuRenderPassEncoderEnd(internal_data->handle);
    wgpuRenderPassEncoderRelease(internal_data->handle);

    return true;
}

b8 webgpu_renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices) {
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
        old_range.index_element_size = internal_data->index_element_size;
        old_range.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_range.vertex_count = internal_data->vertex_count;
        old_range.vertex_element_size = internal_data->vertex_element_size;
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
    //PRINT_DEBUG("webgpu_renderer_create_geometry: vertex_size=%d, vertex_count=%d, vertices=%p ", vertex_size, vertex_count, vertices);
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_element_size = sizeof(Vertex3D);
    u32 total_size = vertex_count * vertex_size;

    // Allocate space in the buffer.
    if (!renderer_renderbuffer_allocate(&context.object_vertex_buffer, total_size, &internal_data->vertex_buffer_offset)) {
        PRINT_ERROR("webgpu_renderer_create_geometry failed to allocate from the vertex buffer!");
        return false;
    }

    // Load the data.
    if (!renderer_renderbuffer_load_range(&context.object_vertex_buffer, internal_data->vertex_buffer_offset, total_size, vertices)) {
        PRINT_ERROR("webgpu_renderer_create_geometry failed to upload to the vertex buffer!");
        return false;
    }

    // Index data, if applicable
    if (index_count && indices) {
        internal_data->index_count = index_count;
        internal_data->index_element_size = sizeof(u32);
        total_size = index_count * index_size;
        if (!renderer_renderbuffer_allocate(&context.object_index_buffer, total_size, &internal_data->index_buffer_offset)) {
            PRINT_ERROR("webgpu_renderer_create_geometry failed to allocate from the index buffer!");
            return false;
        }

        if (!renderer_renderbuffer_load_range(&context.object_index_buffer, internal_data->index_buffer_offset, total_size, indices)) {
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
        if (!renderer_renderbuffer_free(&context.object_vertex_buffer, old_range.vertex_element_size * old_range.vertex_count, old_range.vertex_buffer_offset)) {
            PRINT_ERROR("webgpu_renderer_create_geometry free operation failed during reupload of vertex data.");
            return false;
        }
        // Free index data, if applicable
        if (old_range.index_element_size > 0) {
            if (!renderer_renderbuffer_free(&context.object_index_buffer, old_range.index_element_size * old_range.index_count, old_range.index_buffer_offset)) {
                PRINT_ERROR("webgpu_renderer_create_geometry free operation failed during reupload of index data.");
                return false;
            }
        }
    }

    return true;
}

void webgpu_renderer_destroy_geometry(GEOMETRY* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        wgpuDevicePoll(context.device, true, NULL);
        WEBGPU_GEOMETRY_DATA* internal_data = &context.geometries[geometry->internal_id];

        // Free vertex data
        if (!renderer_renderbuffer_free(&context.object_vertex_buffer, internal_data->vertex_element_size * internal_data->vertex_count, internal_data->vertex_buffer_offset)) {
            PRINT_ERROR("webgpu_renderer_destroy_geometry failed to free vertex buffer range.");
        }

        // Free index data, if applicable
        if (internal_data->index_element_size > 0) {
            if (!renderer_renderbuffer_free(&context.object_index_buffer, internal_data->index_element_size * internal_data->index_count, internal_data->index_buffer_offset)) {
                PRINT_ERROR("webgpu_renderer_destroy_geometry failed to free index buffer range.");
            }
        }

        // Clean up data.
        yzero_memory(internal_data, sizeof(WEBGPU_GEOMETRY_DATA));
        internal_data->id = INVALID_ID;
        internal_data->generation = INVALID_ID;
    }
}

void webgpu_renderer_draw_geometry(GEOMETRY_RENDER_DATA* data) {
    // Ignore non-uploaded geometries.
    if (data->geometry && data->geometry->internal_id == INVALID_ID) {
        return;
    }

    WEBGPU_GEOMETRY_DATA* buffer_data = &context.geometries[data->geometry->internal_id];
    // Set vertex buffer while encoding the render pass
    
    b8 includes_index_data = buffer_data->index_count > 0;
    if (!webgpu_buffer_draw(&context.object_vertex_buffer, buffer_data->vertex_buffer_offset, buffer_data->vertex_count, includes_index_data)) {
        PRINT_ERROR("webgpu_renderer_draw_geometry failed to draw vertex buffer;");
        return;
    }

    if (includes_index_data) {
        if (!webgpu_buffer_draw(&context.object_index_buffer, buffer_data->index_buffer_offset, buffer_data->index_count, !includes_index_data)) {
            PRINT_ERROR("webgpu_renderer_draw_geometry failed to draw index buffer;");
            return;
        }
    }

}

// The index of the global bind set.
const u32 BIND_GROUP_SET_INDEX_GLOBAL = 0;
// The index of the instance bind set.
const u32 BIND_GROUP_SET_INDEX_INSTANCE = 1;
// The index of the texturex bind set.
// it will take the last bind group index.
//const u32 BIND_GROUP_SET_INDEX_TEXTURES = 2;

//START BINDINGS Of Local
const u32 BINDING_INDEX_OBJECT_MATRIX = 0;
//END BINDINGS Of Local

b8 webgpu_renderer_shader_create(struct SHADER *shader, const SHADER_CONFIG* config, RENDERPASS* pass, u8 stage_count, const char **stage_filenames, E_SHADER_STAGE *stages)
{
    shader->internal_data = yallocate_aligned(sizeof(WEBGPU_SHADER), 8, MEMORY_TAG_RENDERER);

    PRINT_DEBUG("webgpu_renderer_shader_create: Creating shader with %d stages.", stage_count);
    // Translate stages
    WGPUShaderStage webgpu_stages[WEBGPU_SHADER_MAX_STAGES];
    for (u8 i = 0; i < stage_count; ++i) {
        switch (stages[i]) {
            case SHADER_STAGE_FRAGMENT:
                webgpu_stages[i] = WGPUShaderStage_Fragment;
                break;
            case SHADER_STAGE_VERTEX:
                webgpu_stages[i] = WGPUShaderStage_Vertex;
                break;
            case SHADER_STAGE_COMPUTE:
                PRINT_WARNING("webgpu_renderer_shader_create: SHADER_STAGE_COMPUTE is set but not yet supported.");
                webgpu_stages[i] = WGPUShaderStage_Compute;
                break;
            default:
                PRINT_ERROR("Unsupported stage type: %d", stages[i]);
                break;
        }
    }

    // TODO: configurable max descriptor allocate count.

    u32 max_bind_allocate_count = 1024;

    // Take a copy of the pointer to the context.
    WEBGPU_SHADER* internal_shader = (WEBGPU_SHADER*)shader->internal_data;
    internal_shader->renderpass = (WEBGPU_RENDERPASS*)pass->internal_data;

    // Build out the configuration.
    internal_shader->config.max_bind_group_count = max_bind_allocate_count;

    // Shader stages. Parse out the flags.
    yzero_memory(internal_shader->config.stages, sizeof(WEBGPU_SHADER_STAGE_CONFIG) * WEBGPU_SHADER_MAX_STAGES);
    internal_shader->config.stage_count = 0;
    // Iterate provided stages.
    for (u32 i = 0; i < stage_count; i++) {
        // Make sure there is room enough to add the stage.
        if (internal_shader->config.stage_count + 1 > WEBGPU_SHADER_MAX_STAGES) {
            PRINT_ERROR("WebGPU Shaders may have a maximum of %d stages", WEBGPU_SHADER_MAX_STAGES);
            return false;
        }

        // Make sure the stage is a supported one.
        WGPUShaderStage stage_flag;
        switch (stages[i]) {
            case SHADER_STAGE_VERTEX:
                stage_flag = WGPUShaderStage_Vertex;
                break;
            case SHADER_STAGE_FRAGMENT:
                stage_flag = WGPUShaderStage_Fragment;
                break;
            default:
                // Go to the next type.
                PRINT_ERROR("webgpu_shader_create: Unsupported shader stage flagged: %d. Stage ignored.", stages[i]);
                continue;
        }

        // Set the stage and bump the counter.
        internal_shader->config.stages[internal_shader->config.stage_count].stage = stage_flag;
        string_ncopy(internal_shader->config.stages[internal_shader->config.stage_count].file_name, stage_filenames[i], 255);
        internal_shader->config.stage_count++;
    }

    // Get the uniform counts.
    internal_shader->global_uniform_count = 0;
    internal_shader->global_uniform_sampler_count = 0;
    internal_shader->instance_uniform_count = 0;
    internal_shader->instance_uniform_sampler_count = 0;
    internal_shader->local_uniform_count = 0;
    u32 total_count = darray_length(config->uniforms);
    for (u32 i = 0; i < total_count; ++i) {
        switch (config->uniforms[i].scope) {
            case SHADER_SCOPE_GLOBAL:
                if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER || config->uniforms[i].type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER) {
                    internal_shader->global_uniform_sampler_count++;
                } else {
                    internal_shader->global_uniform_count++;
                }
                break;
            case SHADER_SCOPE_INSTANCE:
                if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER || config->uniforms[i].type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER) {
                    internal_shader->instance_uniform_sampler_count++;
                } else {
                    internal_shader->instance_uniform_count++;
                }
                break;
            case SHADER_SCOPE_LOCAL:
                internal_shader->local_uniform_count++;
                break;
        }
    }

    
    
    if (internal_shader->global_uniform_count > 0) {
        // Global descriptor set config.
        WGPUBindGroupLayoutDescriptor global_bind_group_layout_desc = {0};
        global_bind_group_layout_desc.nextInChain = NULL;
        global_bind_group_layout_desc.label = (WGPUStringView){"Global object shader bind group descriptor", sizeof("Global object shader bind group descriptor")};
        global_bind_group_layout_desc.entryCount = 0;
        // Define binding layout
        WGPUBindGroupLayoutEntry* binding_layout = yallocate_aligned(sizeof(WGPUBindGroupLayoutEntry), 8, MEMORY_TAG_RENDERER);
        // The binding index as used in the @binding attribute in the shader
        webgpu_bind_layout_set_default(binding_layout);
        // Global UBO binding is first, if present.
        u8 binding_index = global_bind_group_layout_desc.entryCount;
        binding_layout->nextInChain = NULL;
        binding_layout->binding = binding_index;
        binding_layout->buffer.type = WGPUBufferBindingType_Uniform;
        binding_layout->buffer.minBindingSize = 0;
        binding_layout->buffer.hasDynamicOffset = false;
        binding_layout->visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

        global_bind_group_layout_desc.entries = binding_layout;
        global_bind_group_layout_desc.entryCount += 1;
        // Increment the set counter.
        internal_shader->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_GLOBAL] = global_bind_group_layout_desc;
        internal_shader->config.bind_group_count++;
    }

    // If using instance uniforms, add a UBO descriptor set.
    if (internal_shader->instance_uniform_count > 0) {
        // If using instances, add a second descriptor set.
        WGPUBindGroupLayoutDescriptor instance_bind_group_layout_desc = {0};
        instance_bind_group_layout_desc.nextInChain = NULL;
        instance_bind_group_layout_desc.label = (WGPUStringView){"Instance object shader bind group descriptor", sizeof("Instance object shader bind group descriptor")};
        instance_bind_group_layout_desc.entryCount = 0;
        instance_bind_group_layout_desc.entries = NULL;
        // Add a UBO to it, as instances should always have one available.
        // NOTE: Might be a good idea to only add this if it is going to be used...
        
        u8 binding_index = instance_bind_group_layout_desc.entryCount;
        webgpu_bind_layout_set_default(&internal_shader->config.instance_bind_group_entries[binding_index]);
        internal_shader->config.instance_bind_group_entries[binding_index].nextInChain = NULL;
        internal_shader->config.instance_bind_group_entries[binding_index].binding = binding_index;
        internal_shader->config.instance_bind_group_entries[binding_index].buffer.type = WGPUBufferBindingType_Uniform;
        internal_shader->config.instance_bind_group_entries[binding_index].buffer.minBindingSize = 0;
        internal_shader->config.instance_bind_group_entries[binding_index].buffer.hasDynamicOffset = 0;
        internal_shader->config.instance_bind_group_entries[binding_index].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

        instance_bind_group_layout_desc.entries = internal_shader->config.instance_bind_group_entries;
        instance_bind_group_layout_desc.entryCount += 1;

        internal_shader->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_INSTANCE] = instance_bind_group_layout_desc;
        internal_shader->config.bind_group_count++;
    }

    // Add a binding for Samplers if used.
    if (internal_shader->global_uniform_sampler_count > 0 || internal_shader->instance_uniform_sampler_count > 0) {
        WGPUBindGroupLayoutDescriptor textures_bind_group_layout_desc = {0};
        textures_bind_group_layout_desc.nextInChain = NULL;
        textures_bind_group_layout_desc.label = (WGPUStringView){"Textures object shader bind group descriptor", sizeof("Textures object shader bind group descriptor")};

        textures_bind_group_layout_desc.entries = internal_shader->config.textures_bind_group_entries;
        textures_bind_group_layout_desc.entryCount = 0;

        internal_shader->config.bind_group_count++;
        internal_shader->config.bind_group_layout_desc[internal_shader->config.bind_group_count-1] = textures_bind_group_layout_desc;
    }
    // Invalidate all instance states.
    // TODO: dynamic
    for (u32 i = 0; i < 1024; ++i) {
        internal_shader->instance_states[i].id = INVALID_ID;
    }

    // Keep a copy of the cull mode.
    internal_shader->config.cull_mode = config->cull_mode;

    return true;
}

void webgpu_renderer_shader_destroy(struct SHADER *s)
{
    if (s && s->internal_data) {
        WEBGPU_SHADER* shader = s->internal_data;
        if (!shader) {
            PRINT_ERROR("webgpu_renderer_shader_destroy requires a valid pointer to a shader.");
            return;
        }
        
        if (shader->global_bind_group) {
            wgpuBindGroupRelease(shader->global_bind_group);
        }

        if (shader->instance_states->instance_bind_state.bind_group) {
            wgpuBindGroupRelease(shader->instance_states->instance_bind_state.bind_group);
        }

        if (shader->instance_states->instance_bind_state.textures_bind_group) {
            wgpuBindGroupRelease(shader->instance_states->instance_bind_state.textures_bind_group);
        }
        
        // bind groups layouts.
        for (u32 i = 0; i < shader->config.bind_group_count; ++i) {
            
            if (shader->bind_group_layouts[i]) {
                wgpuBindGroupLayoutRelease(shader->bind_group_layouts[i]);
                shader->bind_group_layouts[i] = 0;
            }
        }

        // Uniform buffer.
        //wgpuBufferUnmap(shader->uniform_buffer.handle);
        // Uniform buffer.
        renderer_renderbuffer_destroy(&shader->uniform_buffer);
        renderer_renderbuffer_destroy(&shader->uniform_buffer_staging);

        // Pipeline
        webgpu_pipeline_destroy(&context, &shader->pipeline);

        // Shader modules
        for (u32 i = 0; i < shader->config.stage_count; ++i) {
            wgpuShaderModuleRelease(shader->shader_module[i]);
        }

        // Destroy the configuration.
        yzero_memory(&shader->config, sizeof(WEBGPU_SHADER_CONFIG));

        // Free the internal data memory.
        yfree(s->internal_data);
        s->internal_data = 0;
    }
    
}

b8 webgpu_renderer_shader_init(struct SHADER *shader)
{
    
    WEBGPU_SHADER* shader_internal_data = (WEBGPU_SHADER*)shader->internal_data;

    // TODO: should check if we are loading the same shader module
    // Create a module for each stage.
    yzero_memory(shader_internal_data->shader_module, sizeof(WGPUShaderModule) * WEBGPU_SHADER_MAX_STAGES);
    for (u32 i = 0; i < shader_internal_data->config.stage_count; ++i) {
        if (!webgpu_create_shader_module(&context, &shader_internal_data->shader_module[i], shader_internal_data->config.stages[i].file_name)) {
            PRINT_ERROR("Unable to create %s shader module for '%s'. Shader will be destroyed.", shader_internal_data->config.stages[i].file_name, shader->name);
            return false;
        }
    }


    // Static lookup table for our types->WebGPU ones.
    static WGPUVertexFormat* types = 0;
    static WGPUVertexFormat t[11];
    if (!types) {
        t[SHADER_ATTRIB_TYPE_FLOAT32] = WGPUVertexFormat_Float32;
        t[SHADER_ATTRIB_TYPE_FLOAT32_2] = WGPUVertexFormat_Float32x2;
        t[SHADER_ATTRIB_TYPE_FLOAT32_3] = WGPUVertexFormat_Float32x3;
        t[SHADER_ATTRIB_TYPE_FLOAT32_4] = WGPUVertexFormat_Float32x4;
        t[SHADER_ATTRIB_TYPE_INT8] = WGPUVertexFormat_Sint8;
        t[SHADER_ATTRIB_TYPE_UINT8] = WGPUVertexFormat_Uint8;
        t[SHADER_ATTRIB_TYPE_INT16] = WGPUVertexFormat_Sint16;
        t[SHADER_ATTRIB_TYPE_UINT16] = WGPUVertexFormat_Uint16;
        t[SHADER_ATTRIB_TYPE_INT32] = WGPUVertexFormat_Sint32;
        t[SHADER_ATTRIB_TYPE_UINT32] = WGPUVertexFormat_Uint32;
        types = t;
    }

    // Process attributes
    u32 attribute_count = darray_length(shader->attributes);
    u32 offset = 0;
    for (u32 i = 0; i < attribute_count; ++i) {
        // Setup the new attribute.
        WGPUVertexAttribute attribute;
        attribute.shaderLocation = i;
        attribute.offset = offset;
        attribute.format = types[shader->attributes[i].type];

        // Push into the config's attribute collection and add to the stride.
        shader_internal_data->config.attributes[i] = attribute;

        offset += shader->attributes[i].size;
    }

    WGPUVertexBufferLayout vertex_buffer_layout;
    vertex_buffer_layout.attributeCount = attribute_count;
    vertex_buffer_layout.attributes = shader_internal_data->config.attributes;
    vertex_buffer_layout.arrayStride = shader->attribute_stride;
    vertex_buffer_layout.stepMode = WGPUVertexStepMode_Vertex;

    // Process uniforms.
    u32 uniform_count = darray_length(shader->uniforms);
    u32 sampler_position = 0;
    for (u32 i = 0; i < uniform_count; ++i) {
        // For samplers, the descriptor bindings need to be updated. Other types of uniforms don't need anything to be done here.
        if (shader->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER || shader->uniforms[i].type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER) {
            const u32 set_index = shader_internal_data->config.bind_group_count-1;
            WGPUBindGroupLayoutDescriptor* bind_config = &shader_internal_data->config.bind_group_layout_desc[set_index];

            bind_config->entryCount += 2;
            
            // Define texture binding layout
            // Setup texture binding
            
            webgpu_bind_layout_set_default(&shader_internal_data->config.textures_bind_group_entries[sampler_position]);
            shader_internal_data->config.textures_bind_group_entries[sampler_position].nextInChain = NULL;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].binding = sampler_position;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].sampler.type = WGPUSamplerBindingType_Filtering;

            sampler_position++;

            webgpu_bind_layout_set_default(&shader_internal_data->config.textures_bind_group_entries[sampler_position]);
            shader_internal_data->config.textures_bind_group_entries[sampler_position].nextInChain = NULL;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].binding = sampler_position;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].texture.sampleType = WGPUTextureSampleType_Float;
            shader_internal_data->config.textures_bind_group_entries[sampler_position].texture.viewDimension = (shader->uniforms[i].type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D);

            sampler_position++;
        }
    }

    // Create descriptor set layouts.
    yzero_memory(shader_internal_data->bind_group_layouts, shader_internal_data->config.bind_group_count);
    for (u32 i = 0; i < shader_internal_data->config.bind_group_count; ++i) {
        shader_internal_data->bind_group_layouts[i] = wgpuDeviceCreateBindGroupLayout(context.device, &shader_internal_data->config.bind_group_layout_desc[i]);
        if (!shader_internal_data->bind_group_layouts[i]) {
            PRINT_ERROR("webgpu_shader_init failed creating bind_group_layout");
            return false;
        }
    }

    WGPUVertexState vertex_stage_desc = {0};
    WGPUFragmentState fragment_stage_desc = {0};
    for (u32 i = 0; i < shader_internal_data->config.stage_count; ++i) {
        WGPUShaderStage stage = shader_internal_data->config.stages[i].stage;
        switch (stage)
        {
        case WGPUShaderStage_Vertex:
            vertex_stage_desc.module = shader_internal_data->shader_module[i];
            break;
        case WGPUShaderStage_Fragment:
            fragment_stage_desc.module = shader_internal_data->shader_module[i];
            break;
        
        default:
            PRINT_ERROR("webgpu_renderer_shader_init stage not supported");
            break;
        }
        
    }
    if (!vertex_stage_desc.module){
        PRINT_ERROR("webgpu_renderer_shader_init cannot init without vertex stage in graphics pipeline");
        return false;
    }

    //START Vertex stage setup
    vertex_stage_desc.bufferCount = 1;
    vertex_stage_desc.buffers = &vertex_buffer_layout;
    vertex_stage_desc.entryPoint = (WGPUStringView){ "vs_main", 7 };
    vertex_stage_desc.constantCount = 0;
    vertex_stage_desc.constants = 0;
    //END Vertex stage setup
    
    //START Fragment stage setup
    fragment_stage_desc.entryPoint = (WGPUStringView){ "fs_main", 7 };
    fragment_stage_desc.constantCount = 0;
    fragment_stage_desc.constants = 0;
    // We'll configure the blending stage here
    WGPUBlendState blend_state = {0};
    // Configure color blending equation
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.color.operation = WGPUBlendOperation_Add;
    // Configure alpha blending equation
    blend_state.alpha.srcFactor = WGPUBlendFactor_Zero;
    blend_state.alpha.dstFactor = WGPUBlendFactor_One;
    blend_state.alpha.operation = WGPUBlendOperation_Add;
    WGPUColorTargetState color_target = {0};
    color_target.format = context.swapchain_format;
    color_target.blend = &blend_state;
    color_target.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.
    
    // We have only one target because our render pass has only one output color
    // attachment.
    fragment_stage_desc.targetCount = 1;
    fragment_stage_desc.targets = &color_target;
    //END Fragment stage setup
    
    WEBGPU_PIPELINE_CONFIG pipeline_config = {0};
    pipeline_config.bind_group_layout_count = shader_internal_data->config.bind_group_count;
    pipeline_config.bind_group_layouts = shader_internal_data->bind_group_layouts;
    pipeline_config.vertex_stage = &vertex_stage_desc;
    pipeline_config.fragment_stage = &fragment_stage_desc;
    pipeline_config.cull_mode = shader_internal_data->config.cull_mode;
    pipeline_config.is_wireframe = false;
    pipeline_config.shader_flags = shader->flags;
    pipeline_config.push_constant_range_count = shader->push_constant_range_count;
    pipeline_config.push_constant_ranges = shader->push_constant_ranges;

    b8 pipeline_result = webgpu_pipeline_create(&context, &pipeline_config, &shader_internal_data->pipeline);

    if (!pipeline_result) {
        PRINT_ERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }

    // Grab the UBO alignment requirement from the device.
    shader->required_ubo_alignment = context.device_supported_limits.minUniformBufferOffsetAlignment;

    // Make sure the UBO is aligned according to device requirements.
    shader->global_ubo_stride = get_aligned(shader->global_ubo_size, shader->required_ubo_alignment);
    shader->ubo_stride = get_aligned(shader->ubo_size, shader->required_ubo_alignment);

    // Uniform  buffer.
    // TODO: max count should be configurable, or perhaps long term support of buffer resizing.
    u64 total_buffer_size = shader->global_ubo_stride + (shader->ubo_stride * WEBGPU_MAX_MATERIAL_COUNT);  // global + (locals)
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_UNIFORM, total_buffer_size, true, &shader_internal_data->uniform_buffer)) {
        PRINT_ERROR("WebGPU buffer creation failed for object shader.");
        return false;
    }
    
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_STAGING, total_buffer_size, true, &shader_internal_data->uniform_buffer_staging)) {
        PRINT_ERROR("WebGPU buffer creation failed for object shader.");
        return false;
    }
    // Allocate space for the global UBO, whcih should occupy the _stride_ space, _not_ the actual size used.
    if (!renderer_renderbuffer_allocate(&shader_internal_data->uniform_buffer, shader->global_ubo_stride, &shader->global_ubo_offset)) {
        PRINT_ERROR("Failed to allocate space for the uniform buffer!");
        return false;
    }
    if (!renderer_renderbuffer_allocate(&shader_internal_data->uniform_buffer_staging, shader->global_ubo_stride, &shader->global_ubo_offset)) {
        PRINT_ERROR("Failed to allocate space for the uniform buffer staging!");
        return false;
    }

    // Map the entire buffer's memory.
    //shader_internal_data->mapped_uniform_buffer_block = webgpu_buffer_map_memory(&s->uniform_buffer);
    
    //shader_internal_data->mapped_uniform_buffer_block = wgpuBufferGetMappedRange(shader_internal_data->uniform_buffer_staging.handle, 0, wgpuBufferGetSize(shader_internal_data->uniform_buffer_staging.handle));//wgpuBufferGetMappedRange(shader_internal_data->uniform_buffer.handle, 0, wgpuBufferGetSize(shader_internal_data->uniform_buffer.handle));
    
    // Create a binding
    WGPUBindGroupEntry binding_entry = {0};
    binding_entry.nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding_entry.binding = BIND_GROUP_SET_INDEX_GLOBAL;
    // The buffer it is actually bound to
    binding_entry.buffer = ((WEBGPU_BUFFER*)shader_internal_data->uniform_buffer.internal_data)->handle;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding_entry.offset = shader->global_ubo_offset;
    // And we specify again the size of the buffer.
    binding_entry.size = shader->global_ubo_stride;
    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bind_groupt_desc = {0};
    bind_groupt_desc.nextInChain = NULL;
    bind_groupt_desc.label = (WGPUStringView){ "global bind group", sizeof("global bind group") };
    bind_groupt_desc.layout = shader_internal_data->bind_group_layouts[BIND_GROUP_SET_INDEX_GLOBAL];
    // There must be as many bindings as declared in the layout!
    bind_groupt_desc.entryCount = shader_internal_data->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_GLOBAL].entryCount;
    bind_groupt_desc.entries = &binding_entry;
    shader_internal_data->global_bind_group = wgpuDeviceCreateBindGroup(context.device, &bind_groupt_desc);

    return true;
}

b8 webgpu_renderer_shader_use(struct SHADER *shader)
{
    context.current_shader = shader;
    WEBGPU_SHADER* internal_shader = shader->internal_data;
    wgpuRenderPassEncoderSetPipeline(internal_shader->renderpass->handle, internal_shader->pipeline.handle);
    //map_completed = false;
    webgpu_buffer_map_memory(&internal_shader->uniform_buffer_staging, 0, 0);

    wgpuDevicePoll(context.device, true, NULL);
/*     while (!map_completed){
        
        PRINT_INFO("waiting for mapping");
    } */
    
    return true;
}

b8 webgpu_renderer_shader_bind_globals(struct SHADER *s)
{
    if (!s) {
        return false;
    }

    // Global UBO is always at the beginning, but use this anyway.
    s->bound_ubo_offset = s->global_ubo_offset;
    return true;
}

b8 webgpu_renderer_shader_bind_instance(struct SHADER *s, u32 instance_id)
{
    if (!s) {
        PRINT_ERROR("webgpu_shader_bind_instance requires a valid pointer to a shader.");
        return false;
    }
    WEBGPU_SHADER* internal = s->internal_data;

    s->bound_instance_id = instance_id;
    WEBGPU_SHADER_INSTANCE_STATE* object_state = &internal->instance_states[instance_id];
    s->bound_ubo_offset = object_state->offset;
    return true;
}

b8 webgpu_renderer_shader_apply_globals(struct SHADER *s)
{
    WEBGPU_SHADER* internal = s->internal_data;
    
    u32 global_set_binding_count = internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_GLOBAL].entryCount;
    if (global_set_binding_count > 1) {
        // TODO: There are samplers to be written. Support this.
        global_set_binding_count = 1;
        PRINT_ERROR("Global image samplers are not yet supported.");

    }

    wgpuRenderPassEncoderSetBindGroup(internal->renderpass->handle, BIND_GROUP_SET_INDEX_GLOBAL, internal->global_bind_group, 0, NULL);

    
    return true;
}

b8 webgpu_renderer_shader_apply_instance(struct SHADER *s, b8 needs_update)
{
    WEBGPU_SHADER* internal = s->internal_data;
    if (internal->instance_uniform_count < 1 && internal->instance_uniform_sampler_count < 1) {
        PRINT_ERROR("This shader does not use instances.");
        return false;
    }

    // Obtain instance data.
    WEBGPU_SHADER_INSTANCE_STATE* object_state = &internal->instance_states[s->bound_instance_id];

    if (needs_update) {
        if (internal->instance_uniform_count > 0){
            // A bind group contains one or multiple bindings
            WGPUBindGroupDescriptor instance_bind_group_desc = {0};
            instance_bind_group_desc.nextInChain = NULL;
            instance_bind_group_desc.label = (WGPUStringView){"instance bind group", sizeof("instance bind group")-1};
            instance_bind_group_desc.entryCount = 0;
            instance_bind_group_desc.entries = NULL;

            u32 total_bind_count = internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_INSTANCE].entryCount;
            instance_bind_group_desc.layout = internal->bind_group_layouts[BIND_GROUP_SET_INDEX_INSTANCE];
            u32 bind_index = 0;
            WGPUBindGroupEntry binding[total_bind_count];
            // Binding 0 - Uniform buffer
            binding[bind_index].binding = bind_index;
            binding[bind_index].buffer = ((WEBGPU_BUFFER*)internal->uniform_buffer.internal_data)->handle;
            // We can specify an offset within the buffer, so that a single buffer can hold
            // multiple uniform blocks.
            binding[bind_index].offset = object_state->offset;
            // And we specify again the size of the buffer.
            binding[bind_index].size = s->ubo_stride;
            binding[bind_index].sampler = NULL;
            binding[bind_index].textureView = NULL;
            binding[bind_index].nextInChain = NULL;
            instance_bind_group_desc.entryCount = 1;
            instance_bind_group_desc.entries = binding;
            // Only do this if the descriptor has not yet been updated.
            u8* instance_ubo_generation = &object_state->instance_bind_state.bind_group_states[bind_index].generations;
            // TODO: determine if update is required.
            if (*instance_ubo_generation == INVALID_ID_U8 /*|| *global_ubo_generation != material->generation*/) {
                *instance_ubo_generation = 1;  // material->generation; TODO: some generation from... somewhere
            }
            bind_index++;
            object_state->instance_bind_state.bind_group = wgpuDeviceCreateBindGroup(context.device, &instance_bind_group_desc);
            wgpuRenderPassEncoderSetBindGroup(internal->renderpass->handle, BIND_GROUP_SET_INDEX_INSTANCE, object_state->instance_bind_state.bind_group, 0, NULL);
        }
    }

    if (internal->global_uniform_sampler_count > 0 || internal->instance_uniform_sampler_count > 0) {
        const u32 BIND_GROUP_SET_INDEX_TEXTURES = internal->config.bind_group_count-1;
        u32 textures_total_bind_count = internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_TEXTURES].entryCount;

        if (needs_update){
            WGPUBindGroupEntry binding[textures_total_bind_count];
            // Samplers will always be in the binding. If the binding count is less than 2, there are no samplers.
            if (internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_TEXTURES].entryCount > 1) {
                u32 sampler_count = 0;
                // Iterate samplers.
                for (u32 i = 0; i < textures_total_bind_count; ++i) {
                    if (internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_TEXTURES].entries[i].sampler.type != WGPUSamplerBindingType_BindingNotUsed){
                        sampler_count++;
                    }
                }
                
                //u32 update_sampler_count = 0;
                // increment by 2 because each sampler has a texture after it
                for (u32 i = 0; i < (sampler_count*2); i+= 2) {
                    // TODO: only update in the list if actually needing an update.

                    // i is sampler position which is usually odd number, increase 1 to get to the next even, divide by 2 (because each 2 are one texture), subtract 1 to start index at 0
                    u32 texture_index = ((i)/2);
                    TEXTURE_MAP* map = internal->instance_states[s->bound_instance_id].instance_texture_maps[texture_index];
                    TEXTURE* t = map->texture;

                    // Ensure the texture is valid.
                    if (t->generation == INVALID_ID) {
                        switch (map->use) {
                            case TEXTURE_USE_MAP_DIFFUSE:
                                t = texture_system_get_default_diffuse_texture();
                                break;
                            case TEXTURE_USE_MAP_SPECULAR:
                                t = texture_system_get_default_specular_texture();
                                break;
                            case TEXTURE_USE_MAP_NORMAL:
                                t = texture_system_get_default_normal_texture();
                                break;
                            default:
                                PRINT_WARNING("Undefined texture use %d", map->use);
                                t = texture_system_get_default_texture();
                                break;
                        }
                    }

                    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)t->internal_data;
                    // Assign view and sampler.
                    // Create a binding

                    binding[i].binding = i;
                    binding[i].sampler = *(WGPUSampler*)map->internal_data;
                    binding[i].textureView = NULL;
                    binding[i].buffer = NULL;
                    binding[i].nextInChain = NULL;
                    // The texture is always after the sampler
                    binding[i+1].binding = i+1;
                    binding[i+1].textureView = image->view;
                    binding[i+1].sampler = NULL;
                    binding[i+1].buffer = NULL;
                    binding[i+1].nextInChain = NULL;
                    
                }

            }

            WGPUBindGroupDescriptor instance_textures_bind_group_desc = {0};
            instance_textures_bind_group_desc.nextInChain = NULL;
            instance_textures_bind_group_desc.label = (WGPUStringView){"instance textures bind group", sizeof("instance textures bind group")-1};
            instance_textures_bind_group_desc.layout = internal->bind_group_layouts[BIND_GROUP_SET_INDEX_TEXTURES];
            // There must be as many bindings as declared in the layout!
            instance_textures_bind_group_desc.entryCount = textures_total_bind_count;
            instance_textures_bind_group_desc.entries = binding;
            
            //if (!object_state->instance_bind_state.textures_bind_group){
                object_state->instance_bind_state.textures_bind_group = wgpuDeviceCreateBindGroup(context.device, &instance_textures_bind_group_desc);
            //}
            wgpuDevicePoll(context.device, true, NULL);
        }

        wgpuRenderPassEncoderSetBindGroup(internal->renderpass->handle, BIND_GROUP_SET_INDEX_TEXTURES, object_state->instance_bind_state.textures_bind_group, 0, NULL);
    }
    
    return true;
}

b8 webgpu_renderer_shader_acquire_instance_resources(struct SHADER *s, TEXTURE_MAP** maps, u32 *out_instance_id)
{
    WEBGPU_SHADER* internal = s->internal_data;
    // TODO: dynamic
    *out_instance_id = INVALID_ID;
    for (u32 i = 0; i < 1024; ++i) {
        if (internal->instance_states[i].id == INVALID_ID) {
            internal->instance_states[i].id = i;
            *out_instance_id = i;
            break;
        }
    }
    if (*out_instance_id == INVALID_ID) {
        PRINT_ERROR("webgpu_shader_acquire_instance_resources failed to acquire new id");
        return false;
    }

    WEBGPU_SHADER_INSTANCE_STATE* instance_state = &internal->instance_states[*out_instance_id];
    const u32 BIND_GROUP_SET_INDEX_TEXTURES = internal->config.bind_group_count-1;
    u32 binding_count = internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_TEXTURES].entryCount; 
    u32 instance_texture_count = 0;
    for (u32 i = 0; i < binding_count; i++){
        if (internal->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_TEXTURES].entries[i].sampler.type != WGPUSamplerBindingType_BindingNotUsed){
            instance_texture_count++;
        }
    }

    // Only setup if the shader actually requires it.
    if (s->instance_texture_count > 0) {
        // Wipe out the memory for the entire array, even if it isn't all used.
        instance_state->instance_texture_maps = yallocate_aligned(sizeof(TEXTURE_MAP*) * s->instance_texture_count, 8, MEMORY_TAG_ARRAY);
        TEXTURE* default_texture = texture_system_get_default_texture();
        ycopy_memory(instance_state->instance_texture_maps, maps, sizeof(TEXTURE_MAP*) * s->instance_texture_count);
        // Set unassigned texture pointers to default until assigned.
        for (u32 i = 0; i < instance_texture_count; ++i) {
            if (!maps[i]->texture) {
                instance_state->instance_texture_maps[i]->texture = default_texture;
            }
        }
    }

    // Allocate some space in the UBO - by the stride, not the size.
    u64 size = s->ubo_stride;
    if (size > 0) {
        if (!renderer_renderbuffer_allocate(&internal->uniform_buffer, size, &instance_state->offset)) {
            PRINT_ERROR("webgpu_material_shader_acquire_resources failed to acquire ubo space");
            return false;
        }
        if (!renderer_renderbuffer_allocate(&internal->uniform_buffer_staging, size, &instance_state->offset)) {
            PRINT_ERROR("webgpu_material_shader_acquire_resources failed to acquire ubo space");
            return false;
        }
    }

    WEBGPU_SHADER_BIND_GROUP_SET_STATE* set_state = &instance_state->instance_bind_state;
    yzero_memory(set_state->bind_group_states, sizeof(WEBGPU_BIND_GROUP_STATE) * WEBGPU_SHADER_MAX_BINDINGS);
    for (u32 i = 0; i < binding_count; ++i) {
        set_state->bind_group_states[i].generations = INVALID_ID_U8;
        set_state->bind_group_states[i].ids = INVALID_ID;
    }
    


    return true;
}

b8 webgpu_renderer_shader_release_instance_resources(struct SHADER *s, u32 instance_id)
{
    wgpuDevicePoll(context.device, true, NULL);

    WEBGPU_SHADER* internal = s->internal_data;
    WEBGPU_SHADER_INSTANCE_STATE* instance_state = &internal->instance_states[instance_id];


    if (instance_state->instance_texture_maps) {
        yfree(instance_state->instance_texture_maps);
        instance_state->instance_texture_maps = 0;
    }
    if (!renderer_renderbuffer_free(&internal->uniform_buffer, s->ubo_stride, instance_state->offset)) {
        PRINT_ERROR("webgpu_renderer_shader_release_instance_resources failed to free range from RENDER_BUFFER.");
    }
    instance_state->offset = INVALID_ID;
    instance_state->id = INVALID_ID;

    return true;
}

b8 webgpu_renderer_set_uniform(struct SHADER *frontend_shader, struct SHADER_UNIFORM *uniform, const void *value)
{
    WEBGPU_SHADER* internal = frontend_shader->internal_data;
    if (uniform->type == SHADER_UNIFORM_TYPE_SAMPLER || uniform->type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER) {
        if (uniform->scope == SHADER_SCOPE_GLOBAL) {
            frontend_shader->global_texture_maps[uniform->location] = (TEXTURE_MAP*)value;
        } else {
            internal->instance_states[frontend_shader->bound_instance_id].instance_texture_maps[uniform->location] = (TEXTURE_MAP*)value;
        }
    } else {
        if (uniform->scope == SHADER_SCOPE_LOCAL) {
            // Is local, using push constants. Do this immediately.
            wgpuRenderPassEncoderSetPushConstants(
                internal->renderpass->handle,
                WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, // Shader stage visibility
                uniform->offset,          // Byte offset
                uniform->size,            // Byte size
                value                     // Pointer to data
            );
        } else {
            // Map the appropriate memory location and copy the data over.
            WEBGPU_BUFFER* uniform_buffer_staging = internal->uniform_buffer_staging.internal_data;
            u64 addr = (u64)uniform_buffer_staging->mapped_buffer_block;
            addr += frontend_shader->bound_ubo_offset + uniform->offset;
            ycopy_memory((void*)addr, value, uniform->size);
            if (addr) {
            }
            //wgpuBufferUnmap(internal->uniform_buffer_staging.handle);
        }

    }
    return true;
}

b8 webgpu_shader_after_renderpass(struct SHADER *shader) {
    //PRINT_INFO("buffer size after renderpass");
    wgpuDevicePoll(context.device, true, NULL);
    WEBGPU_SHADER* internal = shader->internal_data;
    webgpu_buffer_copy_range(
        &internal->uniform_buffer_staging,
        0,
        &internal->uniform_buffer,
        0,
        wgpuBufferGetSize(((WEBGPU_BUFFER*)internal->uniform_buffer.internal_data)->handle)
    );
    webgpu_buffer_unmap_memory(&internal->uniform_buffer_staging, 0, 0);
    return true;
}

b8 webgpu_renderpass_create(const RENDERPASS_CONFIG* config, RENDERPASS* out_renderpass) {
    out_renderpass->internal_data = yallocate_aligned(sizeof(WEBGPU_RENDERPASS), 8, MEMORY_TAG_RENDERER);
    WEBGPU_RENDERPASS* internal_data = (WEBGPU_RENDERPASS*)out_renderpass->internal_data;

    internal_data->depth = config->depth;
    internal_data->stencil = config->stencil;

    // Describe Render Pass
    WGPURenderPassDescriptor render_pass_desc = {0};
    render_pass_desc.nextInChain = NULL;
    render_pass_desc.timestampWrites = NULL;
    //render_pass_desc.label = "";

    // Attachments.
    WGPURenderPassColorAttachment* color_attachment_descs = darray_create(WGPURenderPassColorAttachment);
    WGPURenderPassDepthStencilAttachment* depth_attachment_descs = darray_create(WGPURenderPassDepthStencilAttachment);

    // Can always just look at the first target since they are all the same (one per frame).
    // render_target* target = &out_renderpass->targets[0];
    for (u32 i = 0; i < config->target.attachment_count; ++i) {
        RENDER_TARGET_ATTACHMENT_CONFIG* attachment_config = &config->target.attachments[i];

        if (attachment_config->type == RENDER_TARGET_ATTACHMENT_TYPE_COLOR) {
            // Color attachment
            b8 do_clear_color = (out_renderpass->clear_flags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;
            
            WGPURenderPassColorAttachment render_pass_color_attachment = {0};
            render_pass_color_attachment.nextInChain = NULL;
            //render_pass_color_attachment->view = context.target_view;
            render_pass_color_attachment.resolveTarget = NULL;
            WGPUColor color = {0};
            color.r = out_renderpass->clear_color.r;
            color.g = out_renderpass->clear_color.g;
            color.b = out_renderpass->clear_color.b;
            color.a = out_renderpass->clear_color.a;
            render_pass_color_attachment.clearValue = color;

            // Determine which load operation to use.
            if (attachment_config->load_operation == RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_DONT_CARE) {
                // If we don't care, the only other thing that needs checking is if the attachment is being cleared.
                render_pass_color_attachment.loadOp = do_clear_color ? WGPULoadOp_Clear : WGPULoadOp_Undefined;
            } else {
                // If we are loading, check if we are also clearing. This combination doesn't make sense, and should be warned about.
                if (attachment_config->load_operation == RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_LOAD) {
                    if (do_clear_color) {
                        PRINT_WARNING("color attachment load operation set to load, but is also set to clear. This combination is invalid, and will err toward clearing. Verify attachment configuration.");
                        render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
                    } else {
                        render_pass_color_attachment.loadOp = WGPULoadOp_Load;
                    }
                } else {
                    PRINT_ERROR("Invalid and unsupported combination of load operation (0x%x) and clear flags (0x%x) for color attachment.", render_pass_color_attachment.loadOp, out_renderpass->clear_flags);
                    return false;
                }
            }
            
            // Determine which store operation to use.
            if (attachment_config->store_operation == RENDER_TARGET_ATTACHMENT_STORE_OPERATION_DONT_CARE) {
                render_pass_color_attachment.storeOp = WGPUStoreOp_Discard;
            } else if (attachment_config->store_operation == RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE) {
                render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
            } else {
                PRINT_ERROR("Invalid store operation (0x%x) set for depth attachment. Check configuration.", attachment_config->store_operation);
                return false;
            }

            // Push to color attachments array.
            darray_push(color_attachment_descs, render_pass_color_attachment);
        } else if (attachment_config->type == RENDER_TARGET_ATTACHMENT_TYPE_DEPTH) {
            WGPURenderPassDepthStencilAttachment depth_stencil_attachment = {0};
            // Depth attachment, if there is one
            b8 do_clear_depth = (out_renderpass->clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
            // We now add a depth/stencil attachment:
            
            // Setup depth/stencil attachment

            // We do the depth buffer in the beginning of the render pass instead of here.
            // The view of the depth texture
            //depth_stencil_attachment->view = depth_image->view;

            // The initial value of the depth buffer, meaning "far"
            depth_stencil_attachment.depthClearValue = 1.0f;
            // we could turn off writing to the depth buffer globally here
            depth_stencil_attachment.depthReadOnly = false;
            // Depth stencil data.

            // Determine which load operation to use.
            if (attachment_config->load_operation == RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_DONT_CARE) {
                // If we don't care, the only other thing that needs checking is if the attachment is being cleared.
                depth_stencil_attachment.depthLoadOp = do_clear_depth ? WGPULoadOp_Clear : WGPULoadOp_Undefined;
            } else {
                // If we are loading, check if we are also clearing. This combination doesn't make sense, and should be warned about.
                if (attachment_config->load_operation == RENDER_TARGET_ATTACHMENT_LOAD_OPERATION_LOAD) {
                    if (do_clear_depth) {
                        PRINT_WARNING("Depth attachment load operation set to load, but is also set to clear. This combination is invalid, and will err toward clearing. Verify attachment configuration.");
                        depth_stencil_attachment.depthLoadOp = WGPULoadOp_Clear;
                    } else {
                        depth_stencil_attachment.depthLoadOp = WGPULoadOp_Load;
                    }
                } else {
                    PRINT_ERROR("Invalid and unsupported combination of load operation (0x%x) and clear flags (0x%x) for depth attachment.", depth_stencil_attachment.depthLoadOp, out_renderpass->clear_flags);
                    return false;
                }
            }

            // Determine which store operation to use.
            if (attachment_config->store_operation == RENDER_TARGET_ATTACHMENT_STORE_OPERATION_DONT_CARE) {
                depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Discard;
            } else if (attachment_config->store_operation == RENDER_TARGET_ATTACHMENT_STORE_OPERATION_STORE) {
                depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
            } else {
                PRINT_ERROR("Invalid store operation (0x%x) set for depth attachment. Check configuration.", attachment_config->store_operation);
                return false;
            }

            // TODO: Configurability for stencil attachments.
            depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Load;
            depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
            depth_stencil_attachment.stencilClearValue = 0;
            depth_stencil_attachment.stencilReadOnly = true;

            // Push to color attachments array.
            darray_push(depth_attachment_descs, depth_stencil_attachment);
        }
    }

    // color attachment
    u32 color_attachment_count = darray_length(color_attachment_descs);
    if (color_attachment_count > 0) {
        render_pass_desc.colorAttachments = color_attachment_descs;
        render_pass_desc.colorAttachmentCount = color_attachment_count;
    } else {
        render_pass_desc.colorAttachments = NULL;
        render_pass_desc.colorAttachmentCount = 0;
    }

    // Depth attachment reference.
    u32 depth_attachment_count = darray_length(depth_attachment_descs);
    if (depth_attachment_count > 0) {
        YASSERT_MSG(depth_attachment_count == 1, "Multiple depth attachments not supported.");
        // Depth stencil data.
        render_pass_desc.depthStencilAttachment = depth_attachment_descs;
    } else {

        render_pass_desc.depthStencilAttachment = NULL;
    }
    
    internal_data->descriptor = render_pass_desc;

    // WE NEED IT ALLOCATED HERE, BECAUSE IT IS USED IN THE RENDERPASS.
/*     if (color_attachment_descs) {
        darray_destroy(color_attachment_descs);
    }

    if (depth_attachment_descs) {
        darray_destroy(depth_attachment_descs);
    } */
   return true;
}

void webgpu_renderpass_destroy(RENDERPASS* pass) {
    if (pass && pass->internal_data) {
        WEBGPU_RENDERPASS* internal_data = pass->internal_data;
        WGPURenderPassColorAttachment* color_attachment = (WGPURenderPassColorAttachment*)(internal_data->descriptor.colorAttachments);
        if (color_attachment) {
            darray_destroy(color_attachment);
        }
        WGPURenderPassDepthStencilAttachment* depth_stencil_attachment = (WGPURenderPassDepthStencilAttachment*)(internal_data->descriptor.depthStencilAttachment);
        if (depth_stencil_attachment){
            darray_destroy(depth_stencil_attachment);
        }
        wgpuRenderPassEncoderRelease(internal_data->handle);
        yfree(internal_data);
        pass->internal_data = 0;
    }
}

b8 webgpu_renderer_render_target_create(u8 attachment_count, RENDER_TARGET_ATTACHMENT* attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target) {
    ycopy_memory(out_target->attachments, attachments, sizeof(RENDER_TARGET_ATTACHMENT) * attachment_count);

    // no need for an internal framebuffer as these are handled by webgpu.
    out_target->internal_framebuffer = 0;
    return true;
}
void webgpu_renderer_render_target_destroy(RENDER_TARGET* target, b8 free_internal_memory) {
    if (target) {
        target->internal_framebuffer = 0;
        if (free_internal_memory) {
            for (u8 i=0; i < target->attachment_count; ++i) {
                if (target->attachments[i].texture) {
                    // Release the texture view.
                    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)target->attachments[i].texture->internal_data;
                    webgpu_image_destroy(image, target->attachments[i].texture->channel_count, target->attachments[i].texture->type);
                }
            }
            yfree(target->attachments);
            target->attachments = 0;
            target->attachment_count = 0;
        }
    }
}

TEXTURE* webgpu_renderer_window_attachment_get(u8 index) {
/*      if (index >= context.swapchain.image_count) {
        PRINT_ERROR("Attempting to get attachment index out of range: %d. Attachment count: %d", index, context.swapchain.image_count);
        return 0;
    } */
    // we only have one attachment.
    return context.render_texture;
}
TEXTURE* webgpu_renderer_depth_attachment_get(u8 index) {
    return context.depth_texture;
}
u8 webgpu_renderer_window_attachment_index_get(void) {
    // we only have one attachment.
    return 0;
}

u8 webgpu_renderer_window_attachment_count_get(void) {
    // we only have one attachment.
    return 1;
}

b8 webgpu_renderer_is_multithreaded(void) {
    return context.multithreading_enabled;
}

// NOTE: Begin webgpu buffer.

// Indicates if the provided buffer has host-visible memory.
b8 webgpu_buffer_is_host_visible(WEBGPU_BUFFER* buffer) {
    WGPUBufferUsage usage = wgpuBufferGetUsage(buffer->handle);
    return (usage & (WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite));
}

b8 webgpu_buffer_create_internal(RENDER_BUFFER* buffer) {
    if (!buffer) {
        PRINT_ERROR("webgpu_buffer_create_internal requires a valid pointer to a buffer.");
        return false;
    }

    WEBGPU_BUFFER internal_buffer;
    WGPUBufferDescriptor buffer_desc = {0};
    buffer_desc.nextInChain = NULL;
    buffer_desc.usage = 0;
    buffer_desc.size = buffer->total_size;
    buffer_desc.mappedAtCreation = false;
    switch (buffer->type) {
        case RENDERBUFFER_TYPE_VERTEX:
            buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Vertex;
            break;
        case RENDERBUFFER_TYPE_INDEX:
            buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Index;
            break;
        case RENDERBUFFER_TYPE_UNIFORM: {
            buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Uniform;
        } break;
        case RENDERBUFFER_TYPE_STAGING:
            buffer_desc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_MapWrite;
            break;
        case RENDERBUFFER_TYPE_READ:
            buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
            break;
        case RENDERBUFFER_TYPE_STORAGE:
            PRINT_ERROR("Storage buffer not yet supported.");
            return false;
        default:
            PRINT_ERROR("Unsupported buffer type: %i", buffer->type);
            return false;
    }
    char buffer_label[128];
    yzero_memory(buffer_label, sizeof(buffer_label));
    string_format(buffer_label, "Buffer size: %llu, type: %i", buffer->total_size, buffer->type);
    buffer_desc.label = (WGPUStringView){buffer_label, sizeof(buffer_label)};
    internal_buffer.handle = wgpuDeviceCreateBuffer(context.device, &buffer_desc);
    // Report memory as in-use.
    yallocate_report(wgpuBufferGetSize(internal_buffer.handle), MEMORY_TAG_GPU_LOCAL);
    // Allocate the internal state block of memory at the end once we are sure everything was created successfully.
    buffer->internal_data = yallocate_aligned(sizeof(WEBGPU_BUFFER), 8, MEMORY_TAG_WEBGPU);
    *((WEBGPU_BUFFER*)buffer->internal_data) = internal_buffer;

    return true;
}

void webgpu_buffer_destroy_internal(RENDER_BUFFER* buffer) {
    if (buffer) {
        WEBGPU_BUFFER* internal_buffer = (WEBGPU_BUFFER*)buffer->internal_data;
        if (internal_buffer) {
            if (internal_buffer->handle) {
                yfree_report(wgpuBufferGetSize(internal_buffer->handle), MEMORY_TAG_GPU_LOCAL);
                wgpuBufferDestroy(internal_buffer->handle);
                wgpuBufferRelease(internal_buffer->handle);
                internal_buffer->handle = 0;
            }
    
            internal_buffer->mapped_buffer_block = 0;

            // Free up the internal buffer.
            yfree(buffer->internal_data);
            buffer->internal_data = 0;
        }
    }
}

b8 webgpu_buffer_resize(RENDER_BUFFER* buffer, u64 new_size) {
    if (!buffer || !buffer->internal_data) {
        return false;
    }

    WEBGPU_BUFFER* internal_buffer = (WEBGPU_BUFFER*)buffer->internal_data;

    // Create new buffer.
    WGPUBufferDescriptor buffer_desc = {0};
    buffer_desc.nextInChain = NULL;
    buffer_desc.label = (WGPUStringView){"Buffer", sizeof("Buffer")-1};
    buffer_desc.usage = wgpuBufferGetUsage(internal_buffer->handle);
    buffer_desc.size = new_size;
    buffer_desc.mappedAtCreation = false;

    WGPUBuffer new_buffer;
    new_buffer = wgpuDeviceCreateBuffer(context.device, &buffer_desc);

    wgpuDevicePoll(context.device, true, NULL);

    // Copy over the data.
    if (context.encoder){
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(context.device, NULL);

        wgpuCommandEncoderCopyBufferToBuffer(encoder,
            internal_buffer->handle,
            0,
            new_buffer,
            0,
            buffer->total_size);

        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, NULL);
        wgpuQueueSubmit(context.queue, 1, &cmd_buffer);

        wgpuCommandBufferRelease(cmd_buffer);
        wgpuCommandEncoderRelease(encoder);
    } else {
        // If we don't have an encoder, we cannot copy the data.
        PRINT_WARNING("webgpu_buffer_resize requires a valid command encoder to copy data.");
        //return false;
    }

    // Report free of the old, allocate of the new.
    yfree_report(buffer->total_size, MEMORY_TAG_GPU_LOCAL);
    yallocate_report(new_size, MEMORY_TAG_GPU_LOCAL);

    // Destroy the old
    if (internal_buffer->handle) {
        wgpuBufferDestroy(internal_buffer->handle);
        wgpuBufferRelease(internal_buffer->handle);
        internal_buffer->handle = 0;
    }


    // Set new properties
    internal_buffer->handle = new_buffer;
    buffer->total_size = new_size;

    return true;
}

b8 webgpu_buffer_bind(RENDER_BUFFER* buffer, u64 offset) {
    if (!buffer || !buffer->internal_data) {
        PRINT_ERROR("webgpu_buffer_bind requires valid pointer to a buffer.");
        return false;
    }

    // NOTE: Does nothing, for now.
    return true;
}

b8 webgpu_buffer_unbind(RENDER_BUFFER* buffer) {
    if (!buffer || !buffer->internal_data) {
        PRINT_ERROR("webgpu_buffer_unbind requires valid pointer to a buffer.");
        return false;
    }

    // NOTE: Does nothing, for now.
    return true;
}

void on_buffer_map_callback(WGPUMapAsyncStatus status, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
    WEBGPU_BUFFER* internal_buffer = userdata1;
    if (status != WGPUMapAsyncStatus_Success){
        PRINT_ERROR("Failed to map buffer, error: %s", message.data);
    }
    internal_buffer->mapped_buffer_block = wgpuBufferGetMappedRange(internal_buffer->handle, 0, wgpuBufferGetSize(internal_buffer->handle));
}

void* webgpu_buffer_map_memory(RENDER_BUFFER* buffer, u64 offset, u64 size) {
    if (!buffer || !buffer->internal_data) {
        PRINT_ERROR("webgpu_buffer_map_memory requires a valid pointer to a buffer.");
        return 0;
    }
    WEBGPU_BUFFER* internal_buffer = (WEBGPU_BUFFER*)buffer->internal_data;
    WGPUBufferMapCallbackInfo info = {0};
    info.callback = on_buffer_map_callback;
    info.userdata1 = internal_buffer;
    wgpuBufferMapAsync(internal_buffer->handle, WGPUMapMode_Write, offset, wgpuBufferGetSize(internal_buffer->handle), info);
    return internal_buffer->mapped_buffer_block;
}

void webgpu_buffer_unmap_memory(RENDER_BUFFER* buffer, u64 offset, u64 size) {
    if (!buffer || !buffer->internal_data) {
        PRINT_ERROR("webgpu_buffer_unmap_memory requires a valid pointer to a buffer.");
        return;
    }
    WEBGPU_BUFFER* internal_buffer = (WEBGPU_BUFFER*)buffer->internal_data;
    wgpuBufferUnmap(internal_buffer->handle);
}

b8 webgpu_buffer_flush(RENDER_BUFFER* buffer, u64 offset, u64 size) {
    if (!buffer || !buffer->internal_data) {
        PRINT_ERROR("webgpu_buffer_flush requires a valid pointer to a buffer.");
        return false;
    }
    
    // NOTE: Does nothing, for now.
    return true;
}

b8 webgpu_buffer_read(RENDER_BUFFER* buffer, u64 offset, u64 size, void** out_memory) {
    if (!buffer || !buffer->internal_data || !out_memory) {
        PRINT_ERROR("webgpu_buffer_read requires a valid pointer to a buffer and out_memory, and the size must be nonzero.");
        return false;
    }

    WEBGPU_BUFFER* internal_buffer = (WEBGPU_BUFFER*)buffer->internal_data;
    if (!webgpu_buffer_is_host_visible(internal_buffer)) {
        // NOTE: If a read buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
        // create the read buffer, copy data to it, then read from that buffer.

        // Create a host-visible staging buffer to copy to. Mark it as the destination of the transfer.
        RENDER_BUFFER read;
        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_READ, size, false, &read)) {
            PRINT_ERROR("webgpu_buffer_read() - Failed to create read buffer.");
            return false;
        }
        WEBGPU_BUFFER* read_internal = (WEBGPU_BUFFER*)read.internal_data;

        // Perform the copy from device local to the read buffer.
        webgpu_buffer_copy_range(buffer, offset, &read, 0, size);

        // Map/copy/unmap
        WGPUBufferMapCallbackInfo info = {0};
        info.callback = on_buffer_map_callback;
        info.userdata1 = read_internal;
        wgpuBufferMapAsync(read_internal->handle, WGPUMapMode_Read, offset, wgpuBufferGetSize(read_internal->handle), info);
        wgpuDevicePoll(context.device, true, NULL);
        ycopy_memory(*out_memory, read_internal->mapped_buffer_block, size);
        webgpu_buffer_unmap_memory(&read, 0, 0);

        // Clean up the read buffer.
        renderer_renderbuffer_destroy(&read);
    } else {
        // If no staging buffer is needed, map/copy/unmap.
        WGPUBufferMapCallbackInfo info = {0};
        info.callback = on_buffer_map_callback;
        info.userdata1 = internal_buffer;
        wgpuBufferMapAsync(internal_buffer->handle, WGPUMapMode_Read, offset, wgpuBufferGetSize(internal_buffer->handle), info);
        wgpuDevicePoll(context.device, true, NULL);
        ycopy_memory(*out_memory, internal_buffer->mapped_buffer_block, size);
        webgpu_buffer_unmap_memory(buffer, 0, 0);
    }

    return true;
}

b8 webgpu_buffer_load_range(RENDER_BUFFER* buffer, u64 offset, u64 size, const void* data) {
    if (!buffer || !buffer->internal_data || !size || !data) {
        PRINT_ERROR("webgpu_buffer_load_range requires a valid pointer to a buffer, a nonzero size and a valid pointer to data.");
        return false;
    }

    WEBGPU_BUFFER* internal_buffer = (WEBGPU_BUFFER*)buffer->internal_data;
    wgpuQueueWriteBuffer(context.queue, internal_buffer->handle, offset, data, size);

    return true;
}

b8 webgpu_buffer_copy_range(RENDER_BUFFER* source, u64 source_offset, RENDER_BUFFER* dest, u64 dest_offset, u64 size) {
    if (!source || !source->internal_data || !dest || !dest->internal_data || !size) {
        PRINT_ERROR("webgpu_buffer_copy_range requires a valid pointers to source and destination buffers as well as a nonzero size.");
        return false;
    }

    wgpuCommandEncoderCopyBufferToBuffer(context.encoder,
        ((WEBGPU_BUFFER*)source->internal_data)->handle,
        source_offset,
        ((WEBGPU_BUFFER*)dest->internal_data)->handle,
        dest_offset,
        size);
    return true;
}

b8 webgpu_buffer_draw(RENDER_BUFFER* buffer, u64 offset, u32 element_count, b8 bind_only) {
   WEBGPU_SHADER* internal = context.current_shader->internal_data;
   WGPUBuffer buffer_handle = ((WEBGPU_BUFFER*)buffer->internal_data)->handle;
    if (buffer->type == RENDERBUFFER_TYPE_VERTEX) {
        // Bind vertex buffer at offset.
        wgpuRenderPassEncoderSetVertexBuffer(internal->renderpass->handle, 0, buffer_handle, offset, wgpuBufferGetSize(buffer_handle));
        if (!bind_only) {
            wgpuRenderPassEncoderDraw(internal->renderpass->handle, element_count, 1, 0, 0);
        }
        return true;
    } else if (buffer->type == RENDERBUFFER_TYPE_INDEX) {
        // Bind index buffer at offset.
        wgpuRenderPassEncoderSetIndexBuffer(internal->renderpass->handle, buffer_handle, WGPUIndexFormat_Uint32, offset, wgpuBufferGetSize(buffer_handle));
        if (!bind_only) {
            wgpuRenderPassEncoderDrawIndexed(internal->renderpass->handle, element_count, 1, 0, 0, 0);
        }
        return true;
    } else {
        PRINT_ERROR("Cannot draw buffer of type: %i", buffer->type);
        return false;
    }
}

WGPUAddressMode webgpu_convert_repeat_type(const char* axis, E_TEXTURE_REPEAT repeat) {
    switch (repeat) {
        case TEXTURE_REPEAT_REPEAT:
            return WGPUAddressMode_Repeat;
        case TEXTURE_REPEAT_MIRRORED_REPEAT:
            return WGPUAddressMode_MirrorRepeat;
        case TEXTURE_REPEAT_CLAMP_TO_EDGE:
            return WGPUAddressMode_ClampToEdge;
        default:
            PRINT_WARNING("webgpu_convert_repeat_type(axis='%s') Type '%x' not supported, defaulting to repeat.", axis, repeat);
            return WGPUAddressMode_Repeat;
    }
}

WGPUFilterMode webgpu_convert_filter_type(const char* op, E_TEXTURE_FILTER filter) {
    switch (filter) {
        case TEXTURE_FILTER_MODE_NEAREST:
            return WGPUFilterMode_Nearest;
        case TEXTURE_FILTER_MODE_LINEAR:
            return WGPUFilterMode_Linear;
        default:
            PRINT_WARNING("webgpu_convert_filter_type(op='%s'): Unsupported filter type '%x', defaulting to linear.", op, filter);
            return WGPUFilterMode_Linear;
    }
}

b8 webgpu_renderer_texture_map_acquire_resources(TEXTURE_MAP* map){
    // Create a sampler
    WGPUSamplerDescriptor sampler_desc;
    // TODO: These filters should be configurable.
    sampler_desc.nextInChain = NULL;
    sampler_desc.label = (WGPUStringView){"Texture sampler", sizeof("Texture sampler")};
    sampler_desc.magFilter = webgpu_convert_filter_type("mag", map->filter_magnify);
    sampler_desc.minFilter = webgpu_convert_filter_type("min", map->filter_minify);
    sampler_desc.addressModeU = webgpu_convert_repeat_type("U", map->repeat_u);
    sampler_desc.addressModeV = webgpu_convert_repeat_type("V", map->repeat_v);
    sampler_desc.addressModeW = webgpu_convert_repeat_type("W", map->repeat_w);
    sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    sampler_desc.lodMinClamp = 0.0f;
    sampler_desc.lodMaxClamp = 1.0f;
    sampler_desc.compare = WGPUCompareFunction_Undefined;
    sampler_desc.maxAnisotropy = 16;
    map->internal_data = (WGPUSampler*)yallocate_aligned(sizeof(WGPUSampler), 8, MEMORY_TAG_TEXTURE);
    *(WGPUSampler*)map->internal_data = wgpuDeviceCreateSampler(context.device, &sampler_desc);

    if (!map->internal_data) {
        PRINT_ERROR("Failed to create sampler for texture map.");
        return false;
    }

    return true;
}
void webgpu_renderer_texture_map_release_resources(TEXTURE_MAP* map){
    if (map) {
        if (!map->internal_data){
            PRINT_WARNING("webgpu_renderer_texture_map_release_resources: map->internal_data is NULL, nothing to release.");
            return;
        }
        wgpuSamplerRelease(*(WGPUSampler*)map->internal_data);
        yfree(map->internal_data);
        map->internal_data = 0;
    }
}


b8 wgpu_recreate_swapchain(RENDERER_BACKEND* backend) {
    // If already being recreated, do not try again.
    if (context.recreating_swapchain) {
        PRINT_DEBUG("recreate_swapchain called when already recreating. Booting.");
        return false;
    }

    // Detect if the window is worldtoo small to be drawn to
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
        PRINT_DEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
        return false;
    }

    // Mark as recreating if the dimensions are valid.
    context.recreating_swapchain = true;

    wgpuDevicePoll(context.device, true, NULL);

    webgpu_recreate_swapchain(&context, backend, context.framebuffer_width, context.framebuffer_height);

    // Image
    if (!context.render_texture) {
        context.render_texture = yallocate_aligned(sizeof(TEXTURE), 8, MEMORY_TAG_RENDERER);
        void* internal_data = yallocate_aligned(sizeof(WEBGPU_IMAGE), 8, MEMORY_TAG_TEXTURE);
/*         webgpu_image_create(
            &context,
            "__ywmaaengine_default_render_texture__",
            TEXTURE_TYPE_2D,
            context.framebuffer_width,
            context.framebuffer_height,
            WGPUTextureFormat_RGBA8Unorm,
            WGPUTextureUsage_RenderAttachment,
            true,
            WGPUTextureAspect_All,
            internal_data); */
        
        texture_system_wrap_internal(
            "__ywmaaengine_default_render_texture__",
            context.framebuffer_width,
            context.framebuffer_height,
            4,
            false,
            true,
            false,
            internal_data,
            context.render_texture);
        if (!context.render_texture) {
            PRINT_ERROR("Failed to generate new swapchain image texture!");
            return false;
        }
    } else {
        // Just update the dimensions.
        texture_system_resize(context.render_texture, context.framebuffer_width, context.framebuffer_height, false);
    }

    // Depth resources
    if (!webgpu_device_detect_depth_format(&context, false)) {
        context.depth_format = WGPUTextureFormat_Undefined;
        PRINT_ERROR("Failed to find a supported format!");
    }

    if (!context.depth_texture) {
        context.depth_texture = yallocate_aligned(sizeof(TEXTURE), 8, MEMORY_TAG_RENDERER);
    }
    WEBGPU_IMAGE* depth_image = yallocate_aligned(sizeof(TEXTURE), 8, MEMORY_TAG_TEXTURE);
    webgpu_image_create(
        &context,
        "__ywmaaengine_default_depth_texture__",
        TEXTURE_TYPE_2D,
        context.framebuffer_width,
        context.framebuffer_height,
        context.depth_format,
        WGPUTextureUsage_RenderAttachment,
        true,
        WGPUTextureAspect_DepthOnly,
        depth_image);

    // Wrap it in a texture.
    texture_system_wrap_internal(
        "__ywmaaengine_default_depth_texture__",
        context.framebuffer_width,
        context.framebuffer_height,
        context.depth_channel_count,
        false,
        true,
        false,
        depth_image,
        context.depth_texture);
    
    if (!context.depth_texture) {
        PRINT_ERROR("Failed to create depth texture view.");
        return false;
    }

    // Update framebuffer size generation.
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    // Indicate to listeners that a render target refresh is required.
    EVENT_CONTEXT event_context = {0};
    event_fire(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, 0, event_context);

    // Clear the recreating flag.
    context.recreating_swapchain = false;

    return true;
}


WGPUTextureView get_next_surface_texture_view(void) {
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



WGPUTextureFormat webgpu_channel_count_to_format(u8 channel_count, WGPUTextureFormat default_format) {
    switch (channel_count) {
        case 1:
            return WGPUTextureFormat_R8Unorm;
        case 2:
            return WGPUTextureFormat_RG8Unorm;
        case 3:
            return WGPUTextureFormat_RGBA8Unorm;
        case 4:
            return WGPUTextureFormat_RGBA8Unorm;
        default:
            return default_format;
    }
}

void webgpu_renderer_texture_create(const u8* pixels, TEXTURE* texture){
    // Internal data creation.
    // TODO: Use an allocator for this.
    texture->internal_data = (WEBGPU_IMAGE*)yallocate_aligned(sizeof(WEBGPU_IMAGE), 8, MEMORY_TAG_TEXTURE);
    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)texture->internal_data;
    u32 size = texture->width * texture->height * texture->channel_count * (texture->type == TEXTURE_TYPE_CUBE ? 6 : 1);

    // NOTE: Lots of assumptions here, different texture types will require
    // different options here.
    webgpu_image_create(
        &context,
        texture->name,
        texture->type,
        texture->width,
        texture->height,
        WGPUTextureFormat_RGBA8Unorm,
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc | WGPUTextureUsage_RenderAttachment,
        true,
        WGPUTextureAspect_All,
        image);

    // Load the data.
    webgpu_renderer_texture_write_data(texture, 0, size, pixels);

    texture->generation++;
}

void webgpu_renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels) {
    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)t->internal_data;

    // Copy the data from the buffer.
    webgpu_image_copy_from_buffer(&context, t->type, image, pixels, t->channel_count);

    t->generation++;
}

void webgpu_renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height) {
    if (t && t->internal_data) {
        // Resizing is really just destroying the old image and creating a new one.
        // Data is not preserved because there's no reliable way to map the old data to the new
        // since the amount of data differs.
        WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)t->internal_data;
        webgpu_image_destroy(image, t->channel_count, t->type);

        WGPUTextureFormat image_format = webgpu_channel_count_to_format(t->channel_count, WGPUTextureFormat_RGBA8Unorm);

        // NOTE: Lots of assumptions here, different texture types will require
        // different options here.
        webgpu_image_create(
            &context,
            t->name,
            t->type,
            new_width,
            new_height,
            image_format,
            WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc | WGPUTextureUsage_RenderAttachment,
            true,
            WGPUTextureAspect_All,
            image);

        t->generation++;
    }
}

void webgpu_renderer_texture_create_writeable(TEXTURE* t) {
    // Internal data creation.
    t->internal_data = (WEBGPU_IMAGE*)yallocate_aligned(sizeof(WEBGPU_IMAGE), 8, MEMORY_TAG_TEXTURE);
    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)t->internal_data;

    WGPUTextureUsage image_usage;
    WGPUTextureAspect image_aspect;
    WGPUTextureFormat image_format;
    if (t->flags & TEXTURE_FLAG_DEPTH) {
        image_usage = WGPUTextureUsage_RenderAttachment;
        image_aspect = WGPUTextureAspect_DepthOnly;
        image_format = context.depth_format;
    } else {
        image_usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc | WGPUTextureUsage_RenderAttachment;
        image_aspect = WGPUTextureAspect_All;
        image_format = webgpu_channel_count_to_format(t->channel_count, WGPUTextureFormat_RGBA8Unorm);
    }
    // NOTE: Lots of assumptions here, different texture types will require
    // different options here.
    webgpu_image_create(
        &context,
        t->name,
        t->type,
        t->width,
        t->height,
        image_format,
        image_usage,
        true,
        image_aspect,
        image);

    t->generation++;
}

void webgpu_renderer_texture_read_data(TEXTURE* t, u32 offset, u32 size, void** out_memory) {
    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)t->internal_data;

    //WGPUTextureFormat image_format = webgpu_channel_count_to_format(t->channel_count, WGPUTextureFormat_RGBA8Unorm);

    // Create a staging buffer and load data into it.
    RENDER_BUFFER staging;
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_READ, size, false, &staging)) {
        PRINT_ERROR("Failed to create staging buffer for texture read.");
        return;
    }

    // Copy the data to the buffer.
    webgpu_image_copy_to_buffer(t->type, image, t->channel_count, ((WEBGPU_BUFFER*)staging.internal_data)->handle, &context.encoder);

    if (!webgpu_buffer_read(&staging, offset, size, out_memory)) {
        PRINT_ERROR("webgpu_buffer_read failed.");
    }

    renderer_renderbuffer_destroy(&staging);
}

void webgpu_renderer_texture_read_pixel(TEXTURE* t, u32 x, u32 y, u8** out_rgba) {
    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)t->internal_data;

    //WGPUTextureFormat image_format = webgpu_channel_count_to_format(t->channel_count, WGPUTextureFormat_RGBA8Unorm);

    // TODO: creating a buffer every time isn't great. Could optimize this by creating a buffer once
    // and just reusing it.
    // Create a staging buffer and load data into it.
    RENDER_BUFFER staging;
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_READ, 256, false, &staging)) {
        PRINT_ERROR("Failed to create staging buffer for texture pixel read.");
        return;
    }

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(context.device, NULL);

    // Copy the data to the buffer.
    webgpu_image_copy_pixel_to_buffer(t->type, image, t->channel_count, ((WEBGPU_BUFFER*)staging.internal_data)->handle, x, y, &encoder);

    WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, NULL);
    wgpuQueueSubmit(context.queue, 1, &cmd_buffer);

    wgpuCommandBufferRelease(cmd_buffer);
    wgpuCommandEncoderRelease(encoder);


    if (!webgpu_buffer_read(&staging, 0, sizeof(u8) * 4, (void**)out_rgba)) {
        PRINT_ERROR("webgpu_buffer_read failed.");
    }

    renderer_renderbuffer_destroy(&staging);
}

void webgpu_renderer_texture_destroy(TEXTURE* texture){
    wgpuDevicePoll(context.device, true, NULL);

    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)texture->internal_data;

    if (image) {
        webgpu_image_destroy(image, texture->channel_count, texture->type);
        yzero_memory(image, sizeof(WEBGPU_IMAGE));

        yfree(texture->internal_data);
    }
    

    yzero_memory(texture, sizeof(struct TEXTURE));
    
}

void webgpu_bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout) {
    bindingLayout->buffer.nextInChain = NULL;
    bindingLayout->buffer.type = WGPUBufferBindingType_BindingNotUsed;
    bindingLayout->buffer.hasDynamicOffset = false;
    bindingLayout->buffer.minBindingSize = 0;

    bindingLayout->sampler.nextInChain = NULL;
    bindingLayout->sampler.type = WGPUSamplerBindingType_BindingNotUsed;

    bindingLayout->storageTexture.nextInChain = NULL;
    bindingLayout->storageTexture.access = WGPUStorageTextureAccess_BindingNotUsed;
    bindingLayout->storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout->storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout->texture.nextInChain = NULL;
    bindingLayout->texture.multisampled = false;
    bindingLayout->texture.sampleType = WGPUTextureSampleType_BindingNotUsed;
    bindingLayout->texture.viewDimension = WGPUTextureViewDimension_Undefined;
}
#pragma clang optimize on

