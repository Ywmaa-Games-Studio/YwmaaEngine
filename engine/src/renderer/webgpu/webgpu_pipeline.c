#include "webgpu_pipeline.h"
#include "core/logger.h"
#include "io/filesystem.h"
#include "core/ymemory.h"

void webgpu_create_bind_group(WEBGPU_CONTEXT* context);
void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);

b8 webgpu_pipeline_create(WEBGPU_CONTEXT* context){
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
    char* code = yallocate(sizeof(char) * file_size +extra_size, MEMORY_TAG_STRING);
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
    context->shaderModule = wgpuDeviceCreateShaderModule(context->device, &shaderDesc);
    PRINT_INFO("Got shader module: %i", &context->shaderModule);

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

    
    // [...] Describe render pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.nextInChain = NULL;
    // [...] Describe vertex pipeline state
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertex_buffer_layout;
    // NB: We define the 'shaderModule' in the second part of this chapter.
    // Here we tell that the programmable vertex shader stage is described
    // by the function called 'vs_main' in that module.
    pipelineDesc.vertex.module = context->shaderModule;
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
    fragmentState.module = context->shaderModule;
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
    colorTarget.format = context->swapchain_format;
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
    context->pipeline.bind_group_layout = wgpuDeviceCreateBindGroupLayout(context->device, &bindGroupLayoutDesc);
    
    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    //layoutDesc.nextInChain = NULL;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &context->pipeline.bind_group_layout;
    context->pipeline.layout = wgpuDeviceCreatePipelineLayout(context->device, &layoutDesc);
    pipelineDesc.layout = context->pipeline.layout;

    context->pipeline.handle = wgpuDeviceCreateRenderPipeline(context->device, &pipelineDesc);
    wgpuShaderModuleRelease(context->shaderModule);

    WGPUBufferDescriptor uniform_buffer_desc = {};
    uniform_buffer_desc.nextInChain = NULL;
    uniform_buffer_desc.label = "Uniform Buffer";
    uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    uniform_buffer_desc.size = sizeof(GLOBAL_UNIFORM_OBJECT);
    uniform_buffer_desc.mappedAtCreation = false;
    context->global_descriptor = uniform_buffer_desc;
    context->global_uniform_buffer = wgpuDeviceCreateBuffer(context->device, &context->global_descriptor);

    webgpu_create_bind_group(context);

    return true;
}

void webgpu_pipeline_destroy(WEBGPU_CONTEXT* context)
{
    wgpuRenderPipelineRelease(context->pipeline.handle);
    wgpuPipelineLayoutRelease(context->pipeline.layout);
    wgpuBindGroupRelease(context->pipeline.bind_group);
    wgpuBindGroupLayoutRelease(context->pipeline.bind_group_layout);
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
    binding.buffer = context->global_uniform_buffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding.offset = 0;
    // And we specify again the size of the buffer.
    binding.size = sizeof(GLOBAL_UNIFORM_OBJECT);

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.nextInChain = NULL;
    bindGroupDesc.layout = context->pipeline.bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    context->pipeline.bind_group = wgpuDeviceCreateBindGroup(context->device, &bindGroupDesc);
}