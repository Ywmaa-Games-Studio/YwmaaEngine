#pragma clang optimize off // Disable optimizations here because sometimes they cause damage by removing important zeroed variables

#include "webgpu_types.inl"

#include "webgpu_backend.h"
#include "webgpu_platform.h"
#include "webgpu_swapchain.h"
#include "webgpu_device.h"
#include "webgpu_image.h"
#include "webgpu_buffer.h"
#include "webgpu_shader_utils.h"
#include "webgpu_pipeline.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "core/application.h"

#include "data_structures/darray.h"

#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/texture_system.h"
#include "systems/resource_system.h"

void webgpu_bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);

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
    // Free the data range.
    if (buffer) {
        webgpu_buffer_free(buffer, size, offset);
    }
}

b8 webgpu_create_buffers(WEBGPU_CONTEXT* context);
void webgpu_destroy_buffers(WEBGPU_CONTEXT* context);

b8 wgpu_recreate_swapchain(RENDERER_BACKEND* backend);

WGPUTextureView get_next_surface_texture_view(void);
void get_depth_texture(WGPUTexture* out_depth_texture, WGPUTextureView* out_depth_texture_view, u32 width, u32 height);
// static WebGPU context
static WEBGPU_CONTEXT context;


b8 webgpu_renderer_backend_init(RENDERER_BACKEND* backend, const  RENDERER_BACKEND_CONFIG* config, u8* out_window_render_target_count) {
    context.on_rendertarget_refresh_required = config->on_rendertarget_refresh_required;

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

    wgpu_recreate_swapchain(backend);

    // Save off the number of images we have as the number of render targets needed.
    *out_window_render_target_count = 1;

    // Hold registered renderpasses.
    for (u32 i = 0; i < WEBGPU_MAX_REGISTERED_RENDERPASSES; ++i) {
        context.registered_passes[i].id = INVALID_ID_U16;
    }

    // The renderpass table will be a lookup of array indices. Start off every index with an invalid id.
    context.renderpass_table_block = yallocate(sizeof(u32) * WEBGPU_MAX_REGISTERED_RENDERPASSES, MEMORY_TAG_RENDERER);
    hashtable_create(sizeof(u32), WEBGPU_MAX_REGISTERED_RENDERPASSES, context.renderpass_table_block, false, &context.renderpass_table);
    u32 value = INVALID_ID;
    hashtable_fill(&context.renderpass_table, &value);

    // Renderpasses
    for (u32 i = 0; i < config->renderpass_count; ++i) {
        // TODO: move to a function for reusability.
        // Make sure there are no collisions with the name first.
        u32 id = INVALID_ID;
        hashtable_get(&context.renderpass_table, config->pass_configs[i].name, &id);
        if (id != INVALID_ID) {
            PRINT_ERROR("Collision with renderpass named '%s'. Initialization failed.", config->pass_configs[i].name);
            return false;
        }
        // Snip up a new id.
        for (u32 j = 0; j < WEBGPU_MAX_REGISTERED_RENDERPASSES; ++j) {
            if (context.registered_passes[j].id == INVALID_ID_U16) {
                // Found one.
                context.registered_passes[j].id = j;
                id = j;
                break;
            }
        }

        // Verify we got an id
        if (id == INVALID_ID) {
            PRINT_ERROR("No space was found for a new renderpass. Increase WEBGPU_MAX_REGISTERED_RENDERPASSES. Initialization failed.");
            return false;
        }

        // Setup the renderpass.
        context.registered_passes[id].clear_flags = config->pass_configs[i].clear_flags;
        context.registered_passes[id].clear_color = config->pass_configs[i].clear_color;
        context.registered_passes[id].render_area = config->pass_configs[i].render_area;

        webgpu_renderpass_create(&context.registered_passes[id], 1.0f, 0, config->pass_configs[i].prev_name != 0, config->pass_configs[i].next_name != 0);

        // Update the table with the new id.
        hashtable_set(&context.renderpass_table, config->pass_configs[i].name, &id);
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

    // Renderpasses
    for (u32 i = 0; i < WEBGPU_MAX_REGISTERED_RENDERPASSES; ++i) {
        if (context.registered_passes[i].id != INVALID_ID_U16) {
            webgpu_renderpass_destroy(&context.registered_passes[i]);
        }
    }

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

b8 webgpu_renderer_renderpass_begin(RENDERPASS* pass, RENDER_TARGET* target) {
    //START Render Pass

    WEBGPU_RENDERPASS* internal_data = (WEBGPU_RENDERPASS*)pass->internal_data;

    // Max number of attachments
    WGPUTextureView attachment_views[32];
    for (u32 i = 0; i < target->attachment_count; ++i) {
        attachment_views[i] = ((WEBGPU_IMAGE*)target->attachments[i]->internal_data)->view;
    }

    WGPURenderPassColorAttachment* color_attachment = (WGPURenderPassColorAttachment*)(internal_data->descriptor.colorAttachments);
    color_attachment->view = attachment_views[0];
    
    b8 do_clear_depth = (pass->clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (do_clear_depth) {
        WGPURenderPassDepthStencilAttachment* depth_stencil_attachment = (WGPURenderPassDepthStencilAttachment*)(internal_data->descriptor.depthStencilAttachment);
        if (depth_stencil_attachment) {
            // If the depth stencil attachment is not set, we need to create it.
            if (!depth_stencil_attachment->view) {
                depth_stencil_attachment->view = attachment_views[1];
                //get_depth_texture(&depth_stencil_attachment->view, &depth_stencil_attachment->view, context.framebuffer_width, context.framebuffer_height);
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

RENDERPASS* webgpu_renderer_renderpass_get(const char* name) {
    if (!name || name[0] == 0) {
        PRINT_ERROR("webgpu_renderer_renderpass_get requires a name. Nothing will be returned.");
        return 0;
    }

    u32 id = INVALID_ID;
    hashtable_get(&context.renderpass_table, name, &id);
    if (id == INVALID_ID) {
        PRINT_WARNING("There is no registered renderpass named '%s'.", name);
        return 0;
    }

    return &context.registered_passes[id];
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
    if (!webgpu_upload_data_range(&context, &context.object_vertex_buffer, &internal_data->vertex_buffer_offset, total_size, vertices)) {
        PRINT_ERROR("webgpu_renderer_create_geometry failed to upload to the vertex buffer!");
        return false;
    }

    // Index data, if applicable
    if (index_count && indices) {
        internal_data->index_count = index_count;
        internal_data->index_element_size = sizeof(u32);
        total_size = index_count * index_size;
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
        webgpu_free_data_range(&context.object_vertex_buffer, old_range.vertex_buffer_offset, old_range.vertex_element_size * old_range.vertex_count);

        // Free index data, if applicable
        if (old_range.index_element_size > 0) {
            webgpu_free_data_range(&context.object_index_buffer, old_range.index_buffer_offset, old_range.index_element_size  * old_range.index_count);
        }
    }

    return true;
}

void webgpu_renderer_destroy_geometry(GEOMETRY* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        wgpuDevicePoll(context.device, true, NULL);
        WEBGPU_GEOMETRY_DATA* internal_data = &context.geometries[geometry->internal_id];

        // Free vertex data
        webgpu_free_data_range(&context.object_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_element_size * internal_data->vertex_count);

        // Free index data, if applicable
        if (internal_data->index_element_size > 0) {
            webgpu_free_data_range(&context.object_index_buffer, internal_data->index_buffer_offset, internal_data->index_element_size  * internal_data->index_count);
        }

        // Clean up data.
        yzero_memory(internal_data, sizeof(WEBGPU_GEOMETRY_DATA));
        internal_data->id = INVALID_ID;
        internal_data->generation = INVALID_ID;
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
    WEBGPU_SHADER* out_shader = (WEBGPU_SHADER*)shader->internal_data;
    out_shader->renderpass = (WEBGPU_RENDERPASS*)pass->internal_data;

    // Build out the configuration.
    out_shader->config.max_bind_group_count = max_bind_allocate_count;

    // Shader stages. Parse out the flags.
    yzero_memory(out_shader->config.stages, sizeof(WEBGPU_SHADER_STAGE_CONFIG) * WEBGPU_SHADER_MAX_STAGES);
    out_shader->config.stage_count = 0;
    // Iterate provided stages.
    for (u32 i = 0; i < stage_count; i++) {
        // Make sure there is room enough to add the stage.
        if (out_shader->config.stage_count + 1 > WEBGPU_SHADER_MAX_STAGES) {
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
        out_shader->config.stages[out_shader->config.stage_count].stage = stage_flag;
        string_ncopy(out_shader->config.stages[out_shader->config.stage_count].file_name, stage_filenames[i], 255);
        out_shader->config.stage_count++;
    }

    // Get the uniform counts.
    out_shader->global_uniform_count = 0;
    out_shader->global_uniform_sampler_count = 0;
    out_shader->instance_uniform_count = 0;
    out_shader->instance_uniform_sampler_count = 0;
    out_shader->local_uniform_count = 0;
    u32 total_count = darray_length(config->uniforms);
    for (u32 i = 0; i < total_count; ++i) {
        switch (config->uniforms[i].scope) {
            case SHADER_SCOPE_GLOBAL:
                if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER || config->uniforms[i].type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER) {
                    out_shader->global_uniform_sampler_count++;
                } else {
                    out_shader->global_uniform_count++;
                }
                break;
            case SHADER_SCOPE_INSTANCE:
                if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER || config->uniforms[i].type == SHADER_UNIFORM_TYPE_CUBE_SAMPLER) {
                    out_shader->instance_uniform_sampler_count++;
                } else {
                    out_shader->instance_uniform_count++;
                }
                break;
            case SHADER_SCOPE_LOCAL:
                out_shader->local_uniform_count++;
                break;
        }
    }

    
    
    if (out_shader->global_uniform_count > 0) {
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
        out_shader->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_GLOBAL] = global_bind_group_layout_desc;
        out_shader->config.bind_group_count++;
    }

    // If using instance uniforms, add a UBO descriptor set.
    if (out_shader->instance_uniform_count > 0) {
        // If using instances, add a second descriptor set.
        WGPUBindGroupLayoutDescriptor instance_bind_group_layout_desc = {0};
        instance_bind_group_layout_desc.nextInChain = NULL;
        instance_bind_group_layout_desc.label = (WGPUStringView){"Instance object shader bind group descriptor", sizeof("Instance object shader bind group descriptor")};
        instance_bind_group_layout_desc.entryCount = 0;
        instance_bind_group_layout_desc.entries = NULL;
        // Add a UBO to it, as instances should always have one available.
        // NOTE: Might be a good idea to only add this if it is going to be used...
        
        u8 binding_index = instance_bind_group_layout_desc.entryCount;
        webgpu_bind_layout_set_default(&out_shader->config.instance_bind_group_entries[binding_index]);
        out_shader->config.instance_bind_group_entries[binding_index].nextInChain = NULL;
        out_shader->config.instance_bind_group_entries[binding_index].binding = binding_index;
        out_shader->config.instance_bind_group_entries[binding_index].buffer.type = WGPUBufferBindingType_Uniform;
        out_shader->config.instance_bind_group_entries[binding_index].buffer.minBindingSize = 0;
        out_shader->config.instance_bind_group_entries[binding_index].buffer.hasDynamicOffset = 0;
        out_shader->config.instance_bind_group_entries[binding_index].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

        instance_bind_group_layout_desc.entries = out_shader->config.instance_bind_group_entries;
        instance_bind_group_layout_desc.entryCount += 1;

        out_shader->config.bind_group_layout_desc[BIND_GROUP_SET_INDEX_INSTANCE] = instance_bind_group_layout_desc;
        out_shader->config.bind_group_count++;
    }

    // Add a binding for Samplers if used.
    if (out_shader->global_uniform_sampler_count > 0 || out_shader->instance_uniform_sampler_count > 0) {
        WGPUBindGroupLayoutDescriptor textures_bind_group_layout_desc = {0};
        textures_bind_group_layout_desc.nextInChain = NULL;
        textures_bind_group_layout_desc.label = (WGPUStringView){"Textures object shader bind group descriptor", sizeof("Textures object shader bind group descriptor")};

        textures_bind_group_layout_desc.entries = out_shader->config.textures_bind_group_entries;
        textures_bind_group_layout_desc.entryCount = 0;

        out_shader->config.bind_group_count++;
        out_shader->config.bind_group_layout_desc[out_shader->config.bind_group_count-1] = textures_bind_group_layout_desc;
    }
    // Invalidate all instance states.
    // TODO: dynamic
    for (u32 i = 0; i < 1024; ++i) {
        out_shader->instance_states[i].id = INVALID_ID;
    }

    // Keep a copy of the cull mode.
    out_shader->config.cull_mode = config->cull_mode;

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
        shader->mapped_uniform_buffer_block = 0;
        webgpu_buffer_destroy(&shader->uniform_buffer);

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
    WGPURenderPassDepthStencilAttachment* depth_stencil_attachment = (WGPURenderPassDepthStencilAttachment*)(shader_internal_data->renderpass->descriptor.depthStencilAttachment);
    b8 enabled_depth = depth_stencil_attachment != NULL;
    b8 pipeline_result = webgpu_pipeline_create(
        &context,
        shader_internal_data->config.bind_group_count,
        shader_internal_data->bind_group_layouts,
        &vertex_stage_desc,
        &fragment_stage_desc,
        shader->push_constant_range_count,
        shader->push_constant_ranges,
        shader_internal_data->config.cull_mode,
        false,
        enabled_depth, // Enable depth testing
        &shader_internal_data->pipeline);

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
    if (!webgpu_buffer_create(
        &context,
        total_buffer_size,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        false,
        true,
        &shader_internal_data->uniform_buffer)) {
            PRINT_ERROR("WEBGPU global uniform buffer creation failed for object shader.");
            return false;
        }
    if (!webgpu_buffer_create(
        &context,
        total_buffer_size,
        WGPUBufferUsage_CopySrc | WGPUBufferUsage_MapWrite,
        false,
        true,
        &shader_internal_data->uniform_buffer_staging)) {
            PRINT_ERROR("WEBGPU global uniform buffer staging creation failed for object shader.");
            return false;
        }
    // Allocate space for the global UBO, whcih should occupy the _stride_ space, _not_ the actual size used.
    if (!webgpu_buffer_allocate(&shader_internal_data->uniform_buffer, shader->global_ubo_stride, &shader->global_ubo_offset)) {
        PRINT_ERROR("Failed to allocate space for the uniform buffer!");
        return false;
    }

    if (!webgpu_buffer_allocate(&shader_internal_data->uniform_buffer_staging, shader->global_ubo_stride, &shader->global_ubo_offset)) {
        PRINT_ERROR("Failed to allocate space for the uniform buffer!");
        return false;
    }
    
    //shader_internal_data->mapped_uniform_buffer_block = wgpuBufferGetMappedRange(shader_internal_data->uniform_buffer_staging.handle, 0, wgpuBufferGetSize(shader_internal_data->uniform_buffer_staging.handle));//wgpuBufferGetMappedRange(shader_internal_data->uniform_buffer.handle, 0, wgpuBufferGetSize(shader_internal_data->uniform_buffer.handle));
    
    // Create a binding
    WGPUBindGroupEntry binding_entry = {0};
    binding_entry.nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding_entry.binding = BIND_GROUP_SET_INDEX_GLOBAL;
    // The buffer it is actually bound to
    binding_entry.buffer = shader_internal_data->uniform_buffer.handle;
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
//b8 map_completed = false;
void on_buffer_map_callback(WGPUMapAsyncStatus status, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
    WEBGPU_SHADER* s = userdata1;
    if (status != WGPUMapAsyncStatus_Success){
        PRINT_ERROR("Failed to map buffer, error: %s", message.data);
    }
    s->mapped_uniform_buffer_block = wgpuBufferGetMappedRange(s->uniform_buffer_staging.handle, 0, wgpuBufferGetSize(s->uniform_buffer_staging.handle));
    //map_completed = true;
}

b8 webgpu_renderer_shader_use(struct SHADER *shader)
{
    context.current_shader = shader;
    WEBGPU_SHADER* s = shader->internal_data;
    wgpuRenderPassEncoderSetPipeline(s->renderpass->handle, s->pipeline.handle);
    //map_completed = false;
    WGPUBufferMapCallbackInfo info = {0};
    info.callback = on_buffer_map_callback;
    info.userdata1 = s;
    wgpuBufferMapAsync(s->uniform_buffer_staging.handle, WGPUMapMode_Write, 0, wgpuBufferGetSize(s->uniform_buffer_staging.handle), info);

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
            binding[bind_index].buffer = internal->uniform_buffer.handle;
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

    // Allocate some space in the UBO - by the stride, not the size.
    u64 size = s->ubo_stride;
    if (size > 0) {
        if (!webgpu_buffer_allocate(&internal->uniform_buffer, size, &instance_state->offset)) {
            PRINT_ERROR("webgpu_material_shader_acquire_resources failed to acquire ubo space");
            return false;
        }
        if (!webgpu_buffer_allocate(&internal->uniform_buffer_staging, size, &instance_state->offset)) {
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

    webgpu_buffer_free(&internal->uniform_buffer, s->ubo_stride, instance_state->offset);
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
            u64 addr = (u64)internal->mapped_uniform_buffer_block;
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
    wgpuCommandEncoderCopyBufferToBuffer(context.encoder, internal->uniform_buffer_staging.handle, 0, internal->uniform_buffer.handle, 0, wgpuBufferGetSize(internal->uniform_buffer.handle));
    wgpuBufferUnmap(internal->uniform_buffer_staging.handle);
    return true;
}

void webgpu_renderpass_create(RENDERPASS* out_renderpass, f32 depth, u32 stencil, b8 has_prev_pass, b8 has_next_pass) {
    out_renderpass->internal_data = yallocate_aligned(sizeof(WEBGPU_RENDERPASS), 8, MEMORY_TAG_RENDERER);
    WEBGPU_RENDERPASS* internal_data = (WEBGPU_RENDERPASS*)out_renderpass->internal_data;
    internal_data->has_prev_pass = has_prev_pass;
    internal_data->has_next_pass = has_next_pass;

    internal_data->depth = depth;
    internal_data->stencil = stencil;

    // Describe Render Pass
    WGPURenderPassDescriptor render_pass_desc = {0};
    render_pass_desc.nextInChain = NULL;
    //render_pass_desc.label = "";
    render_pass_desc.colorAttachmentCount = 0;
    render_pass_desc.timestampWrites = NULL;

    // Color attachment
    b8 do_clear_color = (out_renderpass->clear_flags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;
    WGPURenderPassColorAttachment* render_pass_color_attachment = yallocate_aligned(sizeof(WGPURenderPassColorAttachment), 8, MEMORY_TAG_RENDERER);
    render_pass_color_attachment->nextInChain = NULL;
    //render_pass_color_attachment->view = context.target_view;
    render_pass_color_attachment->resolveTarget = NULL;
    render_pass_color_attachment->loadOp = (do_clear_color ? WGPULoadOp_Clear : WGPULoadOp_Load);
    render_pass_color_attachment->storeOp = WGPUStoreOp_Store;
    WGPUColor color = {0};
    color.r = out_renderpass->clear_color.r;
    color.g = out_renderpass->clear_color.g;
    color.b = out_renderpass->clear_color.b;
    color.a = out_renderpass->clear_color.a;
    render_pass_color_attachment->clearValue = color;
    // Attachments TODO: make this configurable.
    render_pass_desc.colorAttachments = render_pass_color_attachment;
    render_pass_desc.colorAttachmentCount++;

    // Depth attachment, if there is one
    b8 do_clear_depth = (out_renderpass->clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (do_clear_depth) {
        // We now add a depth/stencil attachment:
        WGPURenderPassDepthStencilAttachment* depth_stencil_attachment = yallocate_aligned(sizeof(WGPURenderPassDepthStencilAttachment), 8, MEMORY_TAG_RENDERER);
        // Setup depth/stencil attachment

        // We do the depth buffer in the beginning of the render pass instead of here.
        // The view of the depth texture
        //WEBGPU_IMAGE* depth_image = (WEBGPU_IMAGE*)context.depth_texture->internal_data;
        //depth_stencil_attachment->view = depth_image->view;

        // The initial value of the depth buffer, meaning "far"
        depth_stencil_attachment->depthClearValue = 1.0f;
        // Operation settings comparable to the color attachment
        if (has_prev_pass) {
            depth_stencil_attachment->depthLoadOp = do_clear_depth ? WGPULoadOp_Clear : WGPULoadOp_Load;
        } else {
            depth_stencil_attachment->depthLoadOp = WGPULoadOp_Undefined;
        }
        depth_stencil_attachment->depthStoreOp = WGPUStoreOp_Store;
        // we could turn off writing to the depth buffer globally here
        depth_stencil_attachment->depthReadOnly = false;

        // Stencil setup, mandatory but unused
        depth_stencil_attachment->stencilClearValue = 0;
        depth_stencil_attachment->stencilLoadOp = WGPULoadOp_Load;
        depth_stencil_attachment->stencilStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment->stencilReadOnly = true;

        // TODO: other attachment types (input, resolve, preserve)

        // Depth stencil data.
        render_pass_desc.depthStencilAttachment = depth_stencil_attachment;
    } else {
        render_pass_desc.depthStencilAttachment = NULL;
    }
    
    internal_data->descriptor = render_pass_desc;
}

void webgpu_renderpass_destroy(RENDERPASS* pass) {
    if (pass && pass->internal_data) {
        WEBGPU_RENDERPASS* internal_data = pass->internal_data;
        wgpuRenderPassEncoderRelease(internal_data->handle);
        yfree((void*)internal_data->descriptor.colorAttachments);
        yfree((void*)internal_data->descriptor.depthStencilAttachment);
        yfree(internal_data);
        pass->internal_data = 0;
    }
}

void webgpu_renderer_render_target_create(u8 attachment_count, TEXTURE** attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target) {
    // Max number of attachments
    WGPUTextureView attachment_views[32];
    for (u32 i = 0; i < attachment_count; ++i) {
        attachment_views[i] = ((WEBGPU_IMAGE*)attachments[i]->internal_data)->view;
    }

    // Take a copy of the attachments and count.
    out_target->attachment_count = attachment_count;
    if (!out_target->attachments) {
        out_target->attachments = yallocate_aligned(sizeof(TEXTURE*) * attachment_count, 8, MEMORY_TAG_ARRAY);
    }
    ycopy_memory(out_target->attachments, attachments, sizeof(TEXTURE*) * attachment_count);

    // no need for an internal framebuffer as these are handled by webgpu.
    out_target->internal_framebuffer = 0;
}
void webgpu_renderer_render_target_destroy(RENDER_TARGET* target, b8 free_internal_memory) {
    if (target) {
        target->internal_framebuffer = 0;
        if (free_internal_memory) {
            for (u8 i=0; i < target->attachment_count; ++i) {
                if (target->attachments[i]) {
                    // Release the texture view.
                    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)target->attachments[i]->internal_data;
                    if (image && image->view) {
                        wgpuTextureViewRelease(image->view);
                        image->view = 0;
                    }
                    if (image->handle) {
                        // Release the texture handle.
                        wgpuTextureDestroy(image->handle);
                        wgpuTextureRelease(image->handle);
                        image->handle = 0;
                    }
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
TEXTURE* webgpu_renderer_depth_attachment_get(void) {
    return context.depth_texture;
}
u8 webgpu_renderer_window_attachment_index_get(void) {
    // we only have one attachment.
    return 0;
}

b8 webgpu_renderer_is_multithreaded(void) {
    return context.multithreading_enabled;
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

void webgpu_renderer_draw_geometry(GEOMETRY_RENDER_DATA* data) {
    // Ignore non-uploaded geometries.
    if (data->geometry && data->geometry->internal_id == INVALID_ID) {
        return;
    }
    
    WEBGPU_SHADER* internal = context.current_shader->internal_data;


    WEBGPU_GEOMETRY_DATA* buffer_data = &context.geometries[data->geometry->internal_id];
    // Set vertex buffer while encoding the render pass
    
    wgpuRenderPassEncoderSetVertexBuffer(internal->renderpass->handle, 0, context.object_vertex_buffer.handle, buffer_data->vertex_buffer_offset, wgpuBufferGetSize(context.object_vertex_buffer.handle));
    // Draw indexed or non-indexed.
    if (buffer_data->index_count > 0) {
        // Bind index buffer at offset.
        wgpuRenderPassEncoderSetIndexBuffer(internal->renderpass->handle, context.object_index_buffer.handle, WGPUIndexFormat_Uint32, buffer_data->index_buffer_offset, wgpuBufferGetSize(context.object_index_buffer.handle));
        // Issue the draw.
        wgpuRenderPassEncoderDrawIndexed(internal->renderpass->handle, buffer_data->index_count, 1, 0, 0, 0);
    } else {
        wgpuRenderPassEncoderDraw(internal->renderpass->handle, buffer_data->vertex_count, 1, 0, 0);
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

        context.render_texture = texture_system_wrap_internal(
            "__ywmaaengine_default_render_texture__",
            context.framebuffer_width,
            context.framebuffer_height,
            4,
            false,
            true,
            false,
            internal_data);
        if (!context.render_texture) {
            PRINT_ERROR("Failed to generate new swapchain image texture!");
            return false;
        }
    } else {
        // Just update the dimensions.
        texture_system_resize(context.render_texture, context.framebuffer_width, context.framebuffer_height, false);
    }

    const u64 candidate_count = 3;
    WGPUTextureFormat candidates[candidate_count] = {
        WGPUTextureFormat_Depth32Float,
        WGPUTextureFormat_Depth32FloatStencil8,
        WGPUTextureFormat_Depth24Plus};
    u8 sizes[candidate_count] = {
        4,
        4,
        3};

    WEBGPU_IMAGE* depth_image = yallocate_aligned(sizeof(TEXTURE), 8, MEMORY_TAG_TEXTURE);
    webgpu_image_create(
        &context,
        "__ywmaaengine_default_depth_texture__",
        TEXTURE_TYPE_2D,
        context.framebuffer_width,
        context.framebuffer_height,
        candidates[2],
        WGPUTextureUsage_RenderAttachment,
        true,
        WGPUTextureAspect_DepthOnly,
        depth_image);

    // Wrap it in a texture.
    context.depth_texture = texture_system_wrap_internal(
        "__ywmaaengine_default_depth_texture__",
        context.framebuffer_width,
        context.framebuffer_height,
        sizes[2], // 4 channels for depth
        false,
        true,
        false,
        depth_image);
    
    if (!context.depth_texture) {
        PRINT_ERROR("Failed to create depth texture view.");
        return false;
    }

    // Update framebuffer size generation.
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

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


b8 webgpu_create_buffers(WEBGPU_CONTEXT* context) {
    const u64 vertex_buffer_size = sizeof(Vertex3D) * 2 * 1024 * 1024;
    if (!webgpu_buffer_create(context, vertex_buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, false, true, &context->object_vertex_buffer)) {
        PRINT_ERROR("Error creating vertex buffer.");
        return false;
    }

    const u64 index_buffer_size = sizeof(u32) * 2 * 1024 * 1024;
    if (!webgpu_buffer_create(context, index_buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index, false, true, &context->object_index_buffer)) {
        PRINT_ERROR("Error creating index buffer.");
        return false;
    }

    
    return true;
}

void webgpu_destroy_buffers(WEBGPU_CONTEXT* context){
    webgpu_buffer_destroy(&context->object_vertex_buffer);
    webgpu_buffer_destroy(&context->object_index_buffer);

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
    u32 size = texture->width * texture->height * texture->channel_count;

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
        webgpu_image_destroy(image);

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

    WGPUTextureFormat image_format = webgpu_channel_count_to_format(t->channel_count, WGPUTextureFormat_RGBA8Unorm);

    // NOTE: Lots of assumptions here, different texture types will require
    // different options here.
    webgpu_image_create(
        &context,
        t->name,
        t->type,
        t->width,
        t->height,
        image_format,
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc | WGPUTextureUsage_RenderAttachment,
        true,
        WGPUTextureAspect_All,
        image);

    t->generation++;
}

void webgpu_renderer_texture_destroy(TEXTURE* texture){
    wgpuDevicePoll(context.device, true, NULL);

    WEBGPU_IMAGE* image = (WEBGPU_IMAGE*)texture->internal_data;

    if (image) {
        webgpu_image_destroy(image);
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

