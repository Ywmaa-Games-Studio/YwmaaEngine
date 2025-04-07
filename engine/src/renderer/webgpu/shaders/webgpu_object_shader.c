#include "webgpu_object_shader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "math/math_types.h"
#include "math/ymath.h"

#include "renderer/webgpu/webgpu_shader_utils.h"
#include "renderer/webgpu/webgpu_pipeline.h"

#define BUILTIN_SHADER_NAME_OBJECT "builtin.shader"

void webgpu_create_bind_group(WEBGPU_CONTEXT* context);
void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);

b8 webgpu_object_shader_create(WEBGPU_CONTEXT* context, WEBGPU_OBJECT_SHADER* out_shader) {


    if (!webgpu_create_shader_module(context, &context->object_shader.shader_module)) {
        //PRINT_ERROR("Unable to create shader module for '%s'.", BUILTIN_SHADER_NAME_OBJECT);
        return false;
    }
    PRINT_INFO("Got shader module: %i", &context->object_shader.shader_module);
    

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
    WGPUBindGroupLayoutEntry binding_layout[3] = {};
    //bind_layout_set_default(&binding_layout);
    //binding_layout.buffer.nextInChain = NULL;
    // The binding index as used in the @binding attribute in the shader
    binding_layout[0].binding = 0;
    binding_layout[0].buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout[0].buffer.minBindingSize = sizeof(GLOBAL_UNIFORM_OBJECT);
    binding_layout[0].visibility = WGPUShaderStage_Vertex;
    
    binding_layout[1].binding = 1;
    binding_layout[1].buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout[1].buffer.minBindingSize = sizeof(Matrice4);
    binding_layout[1].visibility = WGPUShaderStage_Vertex;

    binding_layout[2].binding = 2;
    binding_layout[2].buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout[2].buffer.minBindingSize = sizeof(OBJECT_UNIFORM_OBJECT);
    binding_layout[2].visibility = WGPUShaderStage_Fragment;
    binding_layout[2].buffer.hasDynamicOffset = true;

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.nextInChain = NULL;
    bindGroupLayoutDesc.entryCount = 3;
    bindGroupLayoutDesc.entries = binding_layout;
    context->object_shader.pipeline.bind_group_layout = wgpuDeviceCreateBindGroupLayout(context->device, &bindGroupLayoutDesc);

    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    //layoutDesc.nextInChain = NULL;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &context->object_shader.pipeline.bind_group_layout;
    context->object_shader.pipeline.layout = wgpuDeviceCreatePipelineLayout(context->device, &layoutDesc);

    if (!webgpu_pipeline_create(context, vertex_buffer_layout)) {
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
    context->object_shader.global_buffer_descriptor = uniform_buffer_desc;
    context->object_shader.global_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &context->object_shader.global_buffer_descriptor);

    // Create model uniform buffer.
    WGPUBufferDescriptor model_uniform_buffer_desc = {};
    model_uniform_buffer_desc.nextInChain = NULL;
    model_uniform_buffer_desc.label = "Model Uniform Buffer";
    model_uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    model_uniform_buffer_desc.size = sizeof(OBJECT_UNIFORM_OBJECT);
    model_uniform_buffer_desc.mappedAtCreation = false;
    context->object_shader.model_buffer_descriptor = model_uniform_buffer_desc;
    context->object_shader.model_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &context->object_shader.model_buffer_descriptor);

    // Create object uniform buffer.
    WGPUBufferDescriptor object_uniform_buffer_desc = {};
    object_uniform_buffer_desc.nextInChain = NULL;
    object_uniform_buffer_desc.label = "Object Uniform Buffer";
    object_uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    object_uniform_buffer_desc.size = sizeof(OBJECT_UNIFORM_OBJECT);
    object_uniform_buffer_desc.mappedAtCreation = false;
    context->object_shader.object_buffer_descriptor = object_uniform_buffer_desc;
    context->object_shader.object_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &context->object_shader.object_buffer_descriptor);

    webgpu_create_bind_group(context);

    return true;
}

void webgpu_object_shader_destroy(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader) {
    wgpuBufferDestroy(context->object_shader.global_uniform_buffer);
    webgpu_pipeline_destroy(&context);
    wgpuPipelineLayoutRelease(context->object_shader.pipeline.layout);
    wgpuBindGroupRelease(context->object_shader.pipeline.bind_group);
    wgpuBindGroupLayoutRelease(context->object_shader.pipeline.bind_group_layout);
}

void webgpu_object_shader_use(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader) {
    // Set binding group here!
    u32 dynamic_offsets[3] = {0, 0, sizeof(OBJECT_UNIFORM_OBJECT)};
    wgpuRenderPassEncoderSetBindGroup(context->render_pass, 0, shader->pipeline.bind_group, 1, dynamic_offsets);
}

void webgpu_object_shader_update_global_state(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, f32 delta_time) {
    // Update uniform buffer
    wgpuQueueWriteBuffer(context->queue, context->object_shader.global_uniform_buffer, 0, &shader->global_ubo, sizeof(GLOBAL_UNIFORM_OBJECT));
}

void webgpu_object_shader_update_object(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, GEOMETRY_RENDER_DATA data) {
    wgpuQueueWriteBuffer(context->queue, context->object_shader.model_uniform_buffer, 0, &data.model, sizeof(Matrice4));
    
    
    
    u32 range = sizeof(OBJECT_UNIFORM_OBJECT);
    u64 offset = sizeof(OBJECT_UNIFORM_OBJECT) * data.object_id;  // also the index into the array.
    OBJECT_UNIFORM_OBJECT obo;
    
    // TODO: get diffuse colour from a material.
    static f32 accumulator = 0.0f;
    accumulator += context->frame_delta_time;
    f32 s = (ysin(accumulator) + 1.0f) / 2.0f;  // scale from -1, 1 to 0, 1
    obo.diffuse_color = Vector4_create(s, s, s, 1.0f);
    
    // Load the data into the buffer.
    wgpuQueueWriteBuffer(context->queue, context->object_shader.object_uniform_buffer, offset, &obo, sizeof(OBJECT_UNIFORM_OBJECT));
    
}

b8 webgpu_object_shader_acquire_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32* out_object_id) {
    // TODO: free list


    return true;
}

void webgpu_object_shader_release_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32 object_id) {
    
}

/* void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout) {
    bindingLayout->buffer.nextInChain = NULL;
    bindingLayout->buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayout->buffer.hasDynamicOffset = false;
    bindingLayout->buffer.minBindingSize = sizeof(GLOBAL_UNIFORM_OBJECT);

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
} */

void webgpu_create_bind_group(WEBGPU_CONTEXT* context){
    // Create a binding
    WGPUBindGroupEntry binding[3] = {};
    binding[0].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[0].binding = 0;
    // The buffer it is actually bound to
    binding[0].buffer = context->object_shader.global_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[0].offset = 0;
    // And we specify again the size of the buffer.
    binding[0].size = sizeof(GLOBAL_UNIFORM_OBJECT);

    binding[1].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[1].binding = 1;
    // The buffer it is actually bound to
    binding[1].buffer = context->object_shader.model_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[1].offset = 0;
    // And we specify again the size of the buffer.
    binding[1].size = sizeof(Matrice4);


    binding[2].nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding[2].binding = 2;
    // The buffer it is actually bound to
    binding[2].buffer = context->object_shader.object_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding[2].offset = 0;
    // And we specify again the size of the buffer.
    binding[2].size = sizeof(OBJECT_UNIFORM_OBJECT);

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.nextInChain = NULL;
    bindGroupDesc.layout = context->object_shader.pipeline.bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = 3;
    bindGroupDesc.entries = binding;
    context->object_shader.pipeline.bind_group = wgpuDeviceCreateBindGroup(context->device, &bindGroupDesc);
}