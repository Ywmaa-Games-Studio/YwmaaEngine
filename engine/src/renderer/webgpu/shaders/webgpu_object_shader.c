#include "webgpu_object_shader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "math/math_types.h"
#include "math/ymath.h"

#include "renderer/webgpu/webgpu_shader_utils.h"
#include "renderer/webgpu/webgpu_pipeline.h"

#define BUILTIN_SHADER_NAME_OBJECT "builtin.shader"
#define BINDINGS_ENTRY_COUNT 3
#define TEXTURES_BINDINGS_ENTRY_COUNT 2

void webgpu_create_bind_group(WEBGPU_CONTEXT* context, WEBGPU_OBJECT_SHADER* shader);
void webgpu_create_textures_bind_group(WEBGPU_CONTEXT* context, WGPUTextureView* view, WGPUSampler* sampler, WEBGPU_OBJECT_SHADER* shader);
void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);

b8 webgpu_object_shader_create(WEBGPU_CONTEXT* context, WEBGPU_OBJECT_SHADER* out_shader) {


    if (!webgpu_create_shader_module(context, &out_shader->shader_module)) {
        PRINT_ERROR("Unable to create shader module for '%s'.", BUILTIN_SHADER_NAME_OBJECT);
        return false;
    }
    PRINT_INFO("Got shader module: %i", &out_shader->shader_module);
    

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
    binding_layout[2].buffer.minBindingSize = sizeof(OBJECT_UNIFORM_OBJECT);
    binding_layout[2].visibility = WGPUShaderStage_Fragment;
    binding_layout[2].buffer.hasDynamicOffset = true;

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bind_group_layout_desc;
    bind_group_layout_desc.nextInChain = NULL;
    bind_group_layout_desc.entryCount = sizeof(binding_layout)/sizeof(WGPUBindGroupLayoutEntry);
    bind_group_layout_desc.entries = binding_layout;
    PRINT_INFO("passes");
    out_shader->pipeline.bind_group_layout = wgpuDeviceCreateBindGroupLayout(context->device, &bind_group_layout_desc);
    PRINT_INFO("passes2");

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
    out_shader->pipeline.texture_bind_group_layout = wgpuDeviceCreateBindGroupLayout(context->device, &texture_bind_group_layout_desc);
    PRINT_INFO("fails");
    

    WGPUBindGroupLayout bind_layouts[2] = {};
    bind_layouts[0] = out_shader->pipeline.bind_group_layout;
    bind_layouts[1] = out_shader->pipeline.texture_bind_group_layout;
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
    WGPUBufferDescriptor uniform_buffer_desc = {};
    uniform_buffer_desc.nextInChain = NULL;
    uniform_buffer_desc.label = "Global Uniform Buffer";
    uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    uniform_buffer_desc.size = sizeof(GLOBAL_UNIFORM_OBJECT);
    uniform_buffer_desc.mappedAtCreation = false;
    out_shader->global_buffer_descriptor = uniform_buffer_desc;
    out_shader->global_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &out_shader->global_buffer_descriptor);

    // Create model uniform buffer.
    WGPUBufferDescriptor model_uniform_buffer_desc = {};
    model_uniform_buffer_desc.nextInChain = NULL;
    model_uniform_buffer_desc.label = "Model Uniform Buffer";
    model_uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    model_uniform_buffer_desc.size = sizeof(OBJECT_UNIFORM_OBJECT);
    model_uniform_buffer_desc.mappedAtCreation = false;
    out_shader->model_buffer_descriptor = model_uniform_buffer_desc;
    out_shader->model_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &out_shader->model_buffer_descriptor);

    // Create object uniform buffer.
    WGPUBufferDescriptor object_uniform_buffer_desc = {};
    object_uniform_buffer_desc.nextInChain = NULL;
    object_uniform_buffer_desc.label = "Object Uniform Buffer";
    object_uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    object_uniform_buffer_desc.size = sizeof(OBJECT_UNIFORM_OBJECT);
    object_uniform_buffer_desc.mappedAtCreation = false;
    out_shader->object_buffer_descriptor = object_uniform_buffer_desc;
    out_shader->object_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &out_shader->object_buffer_descriptor);

    webgpu_create_bind_group(context, out_shader);

    return true;
}

void webgpu_object_shader_destroy(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader) {
    wgpuBufferDestroy(shader->object_uniform_buffer);
    wgpuBufferDestroy(shader->model_uniform_buffer);
    wgpuBufferDestroy(shader->global_uniform_buffer);
    webgpu_pipeline_destroy(&context, shader);
    wgpuPipelineLayoutRelease(shader->pipeline.layout);
    wgpuBindGroupRelease(shader->pipeline.bind_group);
    wgpuBindGroupRelease(shader->pipeline.texture_bind_group);
    wgpuBindGroupLayoutRelease(shader->pipeline.bind_group_layout);
    wgpuBindGroupLayoutRelease(shader->pipeline.texture_bind_group_layout);
}

void webgpu_object_shader_use(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader) {
    // Set binding group here!
    u32 dynamic_offsets[3] = {0, 0, sizeof(OBJECT_UNIFORM_OBJECT)};
    wgpuRenderPassEncoderSetBindGroup(context->render_pass, 0, shader->pipeline.bind_group, 1, dynamic_offsets);
}

void webgpu_object_shader_update_global_state(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, f32 delta_time) {
    // Update uniform buffer
    wgpuQueueWriteBuffer(context->queue, shader->global_uniform_buffer, 0, &shader->global_ubo, sizeof(GLOBAL_UNIFORM_OBJECT));
}

void webgpu_object_shader_update_object(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, GEOMETRY_RENDER_DATA data) {
    wgpuQueueWriteBuffer(context->queue, shader->model_uniform_buffer, 0, &data.model, sizeof(Matrice4));
    
    u32 range = sizeof(OBJECT_UNIFORM_OBJECT);
    u64 offset = sizeof(OBJECT_UNIFORM_OBJECT) * data.object_id;  // also the index into the array.
    OBJECT_UNIFORM_OBJECT obo;
    
    // TODO: get diffuse colour from a material.
    static f32 accumulator = 0.0f;
    accumulator += context->frame_delta_time;
    f32 s = (ysin(accumulator) + 1.0f) / 2.0f;  // scale from -1, 1 to 0, 1
    obo.diffuse_color = Vector4_create(s, s, s, 1.0f);

    // Load the data into the buffer.
    wgpuQueueWriteBuffer(context->queue, shader->object_uniform_buffer, offset, &obo, sizeof(OBJECT_UNIFORM_OBJECT));

    // TODO: samplers.
    const u32 sampler_count = 1;
    for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {
        TEXTURE* t = data.textures[sampler_index];
        
        WEBGPU_TEXTURE_DATA* internal_data = (WEBGPU_TEXTURE_DATA*)t->internal_data;
        
        // Assign view and sampler.
        webgpu_create_textures_bind_group(context, &internal_data->image.view, &internal_data->sampler, shader);
        
        
        wgpuRenderPassEncoderSetBindGroup(context->render_pass, 1, shader->pipeline.texture_bind_group, 0, NULL);
    }
    
}

b8 webgpu_object_shader_acquire_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32* out_object_id) {
    // TODO: free list


    return true;
}

void webgpu_object_shader_release_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32 object_id) {
    
}

void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout) {
    bindingLayout->buffer.nextInChain = NULL;
    bindingLayout->buffer.type = WGPUBufferBindingType_Undefined;
    bindingLayout->buffer.hasDynamicOffset = false;
    //bindingLayout->buffer.minBindingSize = 0;

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

void webgpu_create_bind_group(WEBGPU_CONTEXT* context, WEBGPU_OBJECT_SHADER* shader){

    // Create a binding
    WGPUBindGroupEntry binding[BINDINGS_ENTRY_COUNT] = {};
    binding[0].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[0].binding = 0;
    // The buffer it is actually bound to
    binding[0].buffer = shader->global_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[0].offset = 0;
    // And we specify again the size of the buffer.
    binding[0].size = sizeof(GLOBAL_UNIFORM_OBJECT);

    binding[1].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[1].binding = 1;
    // The buffer it is actually bound to
    binding[1].buffer = shader->model_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[1].offset = 0;
    // And we specify again the size of the buffer.
    binding[1].size = sizeof(Matrice4);


    binding[2].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[2].binding = 2;
    // The buffer it is actually bound to
    binding[2].buffer = shader->object_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[2].offset = 0;
    // And we specify again the size of the buffer.
    binding[2].size = sizeof(OBJECT_UNIFORM_OBJECT);

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.nextInChain = NULL;
    bindGroupDesc.layout = shader->pipeline.bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = sizeof(binding)/sizeof(WGPUBindGroupEntry);
    bindGroupDesc.entries = binding;
    shader->pipeline.bind_group = wgpuDeviceCreateBindGroup(context->device, &bindGroupDesc);
}

void webgpu_create_textures_bind_group(WEBGPU_CONTEXT* context, WGPUTextureView* view, WGPUSampler* sampler, WEBGPU_OBJECT_SHADER* shader){
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
    texture_bind_group_desc.layout = shader->pipeline.texture_bind_group_layout;
    // There must be as many bindings as declared in the layout!
    texture_bind_group_desc.entryCount = sizeof(binding)/sizeof(WGPUBindGroupEntry);
    texture_bind_group_desc.entries = binding;

    shader->pipeline.texture_bind_group = wgpuDeviceCreateBindGroup(context->device, &texture_bind_group_desc);
    

}
