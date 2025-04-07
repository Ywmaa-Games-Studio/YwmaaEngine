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
    WGPUBindGroupLayoutEntry binding_layout = {};
    bind_layout_set_default(&binding_layout);
    //binding_layout.buffer.nextInChain = NULL;
    // The binding index as used in the @binding attribute in the shader
    binding_layout.binding = 0;
    binding_layout.buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout.buffer.minBindingSize = sizeof(GLOBAL_UNIFORM_OBJECT);
    binding_layout.visibility = WGPUShaderStage_Vertex;

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.nextInChain = NULL;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &binding_layout;
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

    // Create uniform buffer.
    WGPUBufferDescriptor uniform_buffer_desc = {};
    uniform_buffer_desc.nextInChain = NULL;
    uniform_buffer_desc.label = "Uniform Buffer";
    uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    uniform_buffer_desc.size = sizeof(GLOBAL_UNIFORM_OBJECT);
    uniform_buffer_desc.mappedAtCreation = false;
    context->object_shader.global_descriptor = uniform_buffer_desc;
    context->object_shader.global_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &context->object_shader.global_descriptor);

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
    wgpuRenderPassEncoderSetBindGroup(context->render_pass, 0, shader->pipeline.bind_group, 0, NULL);
}

void webgpu_object_shader_update_global_state(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, f32 delta_time) {
    // Update uniform buffer
    wgpuQueueWriteBuffer(context->queue, context->object_shader.global_uniform_buffer, 0, &shader->global_ubo, sizeof(GLOBAL_UNIFORM_OBJECT));
}

void webgpu_object_shader_update_object(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, GEOMETRY_RENDER_DATA data) {
    
}

b8 webgpu_object_shader_acquire_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32* out_object_id) {
    // TODO: free list


    return true;
}

void webgpu_object_shader_release_resources(WEBGPU_CONTEXT* context, struct WEBGPU_OBJECT_SHADER* shader, u32 object_id) {
    
}

void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout) {
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
}

void webgpu_create_bind_group(WEBGPU_CONTEXT* context){
    // Create a binding
    WGPUBindGroupEntry binding = {};
    binding.nextInChain = NULL;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding.binding = 0;
    // The buffer it is actually bound to
    binding.buffer = context->object_shader.global_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding.offset = 0;
    // And we specify again the size of the buffer.
    binding.size = sizeof(GLOBAL_UNIFORM_OBJECT);

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.nextInChain = NULL;
    bindGroupDesc.layout = context->object_shader.pipeline.bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    context->object_shader.pipeline.bind_group = wgpuDeviceCreateBindGroup(context->device, &bindGroupDesc);
}