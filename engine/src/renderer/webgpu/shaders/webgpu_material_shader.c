#include "webgpu_material_shader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "math/math_types.h"
#include "math/ymath.h"

#include "renderer/webgpu/webgpu_shader_utils.h"
#include "renderer/webgpu/webgpu_pipeline.h"
#include "renderer/webgpu/webgpu_buffer.h"
#include "renderer/webgpu/webgpu_image.h"

#include "systems/texture_system.h"

#define BUILTIN_SHADER_NAME_OBJECT "builtin.shader"
#define BINDINGS_ENTRY_COUNT 3
#define TEXTURES_BINDINGS_ENTRY_COUNT 2

#include "math/ymath.h"

void webgpu_create_bind_group(WEBGPU_CONTEXT* context, WEBGPU_MATERIAL_SHADER* shader);
void webgpu_create_textures_bind_group(WEBGPU_CONTEXT* context, WGPUTextureView* view, WGPUSampler* sampler, WEBGPU_MATERIAL_SHADER* shader, WEBGPU_MATERIAL_SHADER_INSTANCE_STATE* object_state);
void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);

b8 webgpu_material_shader_create(WEBGPU_CONTEXT* context, WEBGPU_MATERIAL_SHADER* out_shader) {


    if (!webgpu_create_shader_module(context, &out_shader->shader_module)) {
        PRINT_ERROR("Unable to create shader module for '%s'.", BUILTIN_SHADER_NAME_OBJECT);
        return false;
    }
    PRINT_INFO("Got shader module: %i", &out_shader->shader_module);
    
    // Sampler uses.
    out_shader->sampler_uses[0] = TEXTURE_USE_MAP_DIFFUSE;

    // Attributes
    WGPUVertexBufferLayout vertex_buffer_layout;
    u32 offset = 0;
    #define ATTRIBUTE_COUNT 2
    WGPUVertexAttribute vertex_attribute[ATTRIBUTE_COUNT];
    // Position, texcoord
    WGPUVertexFormat formats[ATTRIBUTE_COUNT] = {
        WGPUVertexFormat_Float32x3,
        WGPUVertexFormat_Float32x2};
    u64 sizes[ATTRIBUTE_COUNT] = {
        sizeof(Vector3),
        sizeof(Vector2)};
    for (u32 i = 0; i < ATTRIBUTE_COUNT; ++i) {
        vertex_attribute[i].shaderLocation = i;  // attrib location
        vertex_attribute[i].format = formats[i];
        vertex_attribute[i].offset = offset;
        offset += sizes[i];
    }
    
    vertex_buffer_layout.attributeCount = ATTRIBUTE_COUNT;
    vertex_buffer_layout.attributes = vertex_attribute;
    vertex_buffer_layout.arrayStride = sizeof(Vertex3D);
    vertex_buffer_layout.stepMode = WGPUVertexStepMode_Vertex;

    // Define binding layout
    WGPUBindGroupLayoutEntry binding_layout[BINDINGS_ENTRY_COUNT] = {};
    // The binding index as used in the @binding attribute in the shader
    bind_layout_set_default(&binding_layout[0]);
    binding_layout[0].binding = 0;
    binding_layout[0].buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout[0].buffer.minBindingSize = sizeof(GLOBAL_UNIFORM_OBJECT);
    binding_layout[0].visibility = WGPUShaderStage_Vertex;
    
    bind_layout_set_default(&binding_layout[1]);
    binding_layout[1].binding = 1;
    binding_layout[1].buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout[1].buffer.minBindingSize = sizeof(Matrice4);
    binding_layout[1].visibility = WGPUShaderStage_Vertex;
    
    bind_layout_set_default(&binding_layout[2]);
    binding_layout[2].binding = 2;
    binding_layout[2].buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout[2].buffer.minBindingSize = sizeof(MATERIAL_UNIFORM_OBJECT);
    binding_layout[2].visibility = WGPUShaderStage_Fragment;
    binding_layout[2].buffer.hasDynamicOffset = true;

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bind_group_layout_desc;
    bind_group_layout_desc.nextInChain = NULL;
    bind_group_layout_desc.label = "Object shader bind group descriptor";
    bind_group_layout_desc.entryCount = sizeof(binding_layout)/sizeof(WGPUBindGroupLayoutEntry);
    bind_group_layout_desc.entries = binding_layout;

    out_shader->bind_group_layout = wgpuDeviceCreateBindGroupLayout(context->device, &bind_group_layout_desc);

    // Define texture binding layout
    WGPUBindGroupLayoutEntry texture_binding_layout[TEXTURES_BINDINGS_ENTRY_COUNT] = {};

    // Setup texture binding
    bind_layout_set_default(&texture_binding_layout[0]);
    texture_binding_layout[0].binding = 0;
    texture_binding_layout[0].visibility = WGPUShaderStage_Fragment;
    texture_binding_layout[0].texture.sampleType = WGPUTextureSampleType_Float;
    texture_binding_layout[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    
    bind_layout_set_default(&texture_binding_layout[1]);
    texture_binding_layout[1].binding = 1;
    texture_binding_layout[1].visibility = WGPUShaderStage_Fragment;
    texture_binding_layout[1].sampler.type = WGPUSamplerBindingType_Filtering;
    

    // Create a texture bind group layout
    WGPUBindGroupLayoutDescriptor texture_bind_group_layout_desc;
    texture_bind_group_layout_desc.label = "Texture bind group descriptor";
    texture_bind_group_layout_desc.nextInChain = NULL;
    texture_bind_group_layout_desc.entryCount = sizeof(texture_binding_layout)/sizeof(WGPUBindGroupLayoutEntry);
    texture_bind_group_layout_desc.entries = texture_binding_layout;
    out_shader->texture_bind_group_layout = wgpuDeviceCreateBindGroupLayout(context->device, &texture_bind_group_layout_desc);
    

    WGPUBindGroupLayout bind_layouts[2] = {};
    bind_layouts[0] = out_shader->bind_group_layout;
    bind_layouts[1] = out_shader->texture_bind_group_layout;
    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.nextInChain = NULL;
    layoutDesc.bindGroupLayoutCount = sizeof(bind_layouts)/sizeof(WGPUBindGroupLayout);
    layoutDesc.bindGroupLayouts = bind_layouts;
    out_shader->pipeline.layout = wgpuDeviceCreatePipelineLayout(context->device, &layoutDesc);

    if (!webgpu_pipeline_create(context, vertex_buffer_layout, out_shader)) {
        PRINT_ERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }


    // Create global uniform buffer.
    if (!webgpu_buffer_create(
        context,
        sizeof(GLOBAL_UNIFORM_OBJECT),
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        false,
        &out_shader->global_uniform_buffer)) {
    PRINT_ERROR("WEBGPU global uniform buffer creation failed for object shader.");
    return false;
    }

    // Create model uniform buffer.
    if (!webgpu_buffer_create(
        context,
        sizeof(Matrice4),
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        false,
        &out_shader->model_uniform_buffer)) {
    PRINT_ERROR("WEBGPU model uniform buffer creation failed for object shader.");
    return false;
    }

    // Create object uniform buffer.
    if (!webgpu_buffer_create(
        context,
        sizeof(MATERIAL_UNIFORM_OBJECT) * WEBGPU_MAX_MATERIAL_COUNT,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        false,
        &out_shader->object_uniform_buffer)) {
    PRINT_ERROR("WEBGPU object uniform buffer creation failed for object shader.");
    return false;
    }

    webgpu_create_bind_group(context, out_shader);

    return true;
}

void webgpu_material_shader_destroy(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader) {
    webgpu_buffer_destroy(&shader->object_uniform_buffer);
    webgpu_buffer_destroy(&shader->model_uniform_buffer);
    webgpu_buffer_destroy(&shader->global_uniform_buffer); 
    wgpuBindGroupLayoutRelease(shader->texture_bind_group_layout);
    wgpuBindGroupRelease(shader->bind_group);
    wgpuBindGroupLayoutRelease(shader->bind_group_layout);
    webgpu_pipeline_destroy(&context, shader);
    wgpuPipelineLayoutRelease(shader->pipeline.layout);
}

void webgpu_material_shader_use(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader) {
    // Set pipeline before binding groups
    wgpuRenderPassEncoderSetPipeline(context->render_pass, shader->pipeline.handle);
    
    // Set binding group here!
    u32 dynamic_offsets[3] = {0, 0, sizeof(MATERIAL_UNIFORM_OBJECT)};
    wgpuRenderPassEncoderSetBindGroup(context->render_pass, 0, shader->bind_group, 1, dynamic_offsets);
}

void webgpu_material_shader_update_global_state(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader, f32 delta_time) {
    if (!context || !shader) {
        PRINT_ERROR("webgpu_material_shader_update_global_state: context or shader is null.");
        return;
    }
    // Update uniform buffer
    wgpuQueueWriteBuffer(context->queue, shader->global_uniform_buffer.handle, 0, &shader->global_ubo, sizeof(GLOBAL_UNIFORM_OBJECT));
}

void webgpu_material_shader_set_model(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader, Matrice4 model) {
    if (!context || !shader) {
        PRINT_ERROR("webgpu_material_shader_set_model: context or shader is null.");
        return;
    }
    wgpuQueueWriteBuffer(context->queue, shader->model_uniform_buffer.handle, 0, &model, sizeof(Matrice4));
}

void webgpu_material_shader_apply_material(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader, MATERIAL* material) {
    if (!context || !shader) {
        PRINT_ERROR("webgpu_material_shader_apply_material: context or shader is null.");
        return;
    }
    // Obtain material data.
    WEBGPU_MATERIAL_SHADER_INSTANCE_STATE* object_state = &shader->instance_states[material->internal_id];

    u32 range = sizeof(MATERIAL_UNIFORM_OBJECT);
    u64 offset = sizeof(MATERIAL_UNIFORM_OBJECT) * material->internal_id;  // also the index into the array.
    MATERIAL_UNIFORM_OBJECT obo;
    
    // TODO: get diffuse colour from a material.
    // TODO: get diffuse colour from a material.
/*     static f32 accumulator = 0.0f;
    accumulator += context->frame_delta_time;
    f32 s = (ysin(accumulator) + 1.0f) / 2.0f;  // scale from -1, 1 to 0, 1
    obo.diffuse_color = Vector4_create(s, s, s, 1.0f); */
    obo.diffuse_color = material->diffuse_colour;
    //obo.v_reserved0 = Vector4_zero();
    //obo.v_reserved1 = Vector4_zero();
    //obo.v_reserved2 = Vector4_zero();

    // Load the data into the buffer.
    wgpuQueueWriteBuffer(context->queue, shader->object_uniform_buffer.handle, offset, &obo, sizeof(MATERIAL_UNIFORM_OBJECT));

    // Samplers.
    const u32 sampler_count = 1;
    for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {
        E_TEXTURE_USE use = shader->sampler_uses[sampler_index];
        TEXTURE* t = 0;
        switch (use) {
            case TEXTURE_USE_MAP_DIFFUSE:
                t = material->diffuse_map.texture;
                break;
            default:
                PRINT_ERROR("Unable to bind sampler to unknown use.");
                return;
        }

        u32* descriptor_generation = &object_state->texture_bind_state.generations;
        u32* descriptor_id = &object_state->texture_bind_state.ids;

        // If the texture hasn't been loaded yet, use the default.
        if (t->generation == INVALID_ID) {
            t = texture_system_get_default_texture();
        }
        
        
        // Check if the descriptor needs updating first.
        if (t && (*descriptor_id != t->id || *descriptor_generation != t->generation || *descriptor_generation == INVALID_ID)) {
            WEBGPU_TEXTURE_DATA* internal_data = (WEBGPU_TEXTURE_DATA*)t->internal_data;


            // Assign view and sampler.
            webgpu_create_textures_bind_group(context, &internal_data->image.view, &internal_data->sampler, shader, object_state);
            
            // Sync frame generation if not using a default texture.
            if (t->generation != INVALID_ID) {
                *descriptor_generation = t->generation;
                *descriptor_id = t->id;
            }
        }
        
    }
    
    wgpuRenderPassEncoderSetBindGroup(context->render_pass, 1, object_state->texture_bind_group, 0, NULL);
}

b8 webgpu_material_shader_acquire_resources(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader, MATERIAL* material) {
    material->internal_id = shader->object_uniform_buffer_index;
    shader->object_uniform_buffer_index++;
    
    WEBGPU_MATERIAL_SHADER_INSTANCE_STATE* object_state = &shader->instance_states[material->internal_id];
    object_state->texture_bind_state.generations = INVALID_ID;
    object_state->texture_bind_state.ids = INVALID_ID;
    
/*     TEXTURE* t = 0;
    t = texture_system_get_default_texture();
    
    
    // Check if the descriptor needs updating first.
    WEBGPU_TEXTURE_DATA* internal_data = (WEBGPU_TEXTURE_DATA*)t->internal_data;
    
    
    // Assign view and sampler.
    webgpu_create_textures_bind_group(context, &internal_data->image.view, &internal_data->sampler, shader, object_state); */
        

    return true;
}

void webgpu_material_shader_release_resources(WEBGPU_CONTEXT* context, struct WEBGPU_MATERIAL_SHADER* shader, MATERIAL* material) {
    WEBGPU_MATERIAL_SHADER_INSTANCE_STATE* instance_state = &shader->instance_states[material->internal_id];
    
    wgpuBindGroupRelease(instance_state->texture_bind_group);

    instance_state->texture_bind_state.generations = INVALID_ID;
    instance_state->texture_bind_state.ids = INVALID_ID;

    material->internal_id = INVALID_ID; 
}

void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout) {
    bindingLayout->buffer.nextInChain = NULL;
    bindingLayout->buffer.type = WGPUBufferBindingType_Undefined;
    bindingLayout->buffer.hasDynamicOffset = false;
    bindingLayout->buffer.minBindingSize = 0;

    bindingLayout->sampler.nextInChain = NULL;
    bindingLayout->sampler.type = WGPUSamplerBindingType_Undefined;

    bindingLayout->storageTexture.nextInChain = NULL;
    bindingLayout->storageTexture.access = WGPUStorageTextureAccess_Undefined;
    bindingLayout->storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout->storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout->texture.nextInChain = NULL;
    bindingLayout->texture.multisampled = false;
    bindingLayout->texture.sampleType = WGPUTextureSampleType_Undefined;
    bindingLayout->texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

void webgpu_create_bind_group(WEBGPU_CONTEXT* context, WEBGPU_MATERIAL_SHADER* shader){

    // Create a binding
    WGPUBindGroupEntry binding[BINDINGS_ENTRY_COUNT] = {};
    binding[0].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[0].binding = 0;
    // The buffer it is actually bound to
    binding[0].buffer = shader->global_uniform_buffer.handle;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[0].offset = 0;
    // And we specify again the size of the buffer.
    binding[0].size = sizeof(GLOBAL_UNIFORM_OBJECT);

    binding[1].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[1].binding = 1;
    // The buffer it is actually bound to
    binding[1].buffer = shader->model_uniform_buffer.handle;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[1].offset = 0;
    // And we specify again the size of the buffer.
    binding[1].size = sizeof(Matrice4);


    binding[2].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[2].binding = 2;
    // The buffer it is actually bound to
    binding[2].buffer = shader->object_uniform_buffer.handle;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[2].offset = 0;
    // And we specify again the size of the buffer.
    binding[2].size = sizeof(MATERIAL_UNIFORM_OBJECT) * WEBGPU_MAX_MATERIAL_COUNT;

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.nextInChain = NULL;
    bindGroupDesc.label = "bind group";
    bindGroupDesc.layout = shader->bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = sizeof(binding)/sizeof(WGPUBindGroupEntry);
    bindGroupDesc.entries = binding;
    shader->bind_group = wgpuDeviceCreateBindGroup(context->device, &bindGroupDesc);
}

void webgpu_create_textures_bind_group(WEBGPU_CONTEXT* context, WGPUTextureView* view, WGPUSampler* sampler, WEBGPU_MATERIAL_SHADER* shader, WEBGPU_MATERIAL_SHADER_INSTANCE_STATE* object_state){
    // Create a binding
    WGPUBindGroupEntry binding[TEXTURES_BINDINGS_ENTRY_COUNT] = {};
    binding[0].nextInChain = NULL;
    binding[0].binding = 0;
    binding[0].textureView = *view;

    binding[1].nextInChain = NULL;
    binding[1].binding = 1;
    binding[1].sampler = *sampler;

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor texture_bind_group_desc = {};
    texture_bind_group_desc.nextInChain = NULL;
    texture_bind_group_desc.layout = shader->texture_bind_group_layout;
    // There must be as many bindings as declared in the layout!
    texture_bind_group_desc.entryCount = sizeof(binding)/sizeof(WGPUBindGroupEntry);
    texture_bind_group_desc.entries = binding;

    object_state->texture_bind_group = wgpuDeviceCreateBindGroup(context->device, &texture_bind_group_desc);
    

}
