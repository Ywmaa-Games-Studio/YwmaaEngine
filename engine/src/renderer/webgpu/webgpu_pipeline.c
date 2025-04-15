#include "webgpu_pipeline.h"
#include "core/logger.h"
#include "io/filesystem.h"
#include "core/ymemory.h"

void stencil_set_default(WGPUStencilFaceState* stencilFaceState) {
    stencilFaceState->compare = WGPUCompareFunction_Always;
    stencilFaceState->failOp = WGPUStencilOperation_Keep;
    stencilFaceState->depthFailOp = WGPUStencilOperation_Keep;
    stencilFaceState->passOp = WGPUStencilOperation_Keep;
}

void depth_set_default(WGPUDepthStencilState* depthStencilState) {
    depthStencilState->format = WGPUTextureFormat_Undefined;
    depthStencilState->depthWriteEnabled = false;
    depthStencilState->depthCompare = WGPUCompareFunction_Always;
    depthStencilState->stencilReadMask = 0xFFFFFFFF;
    depthStencilState->stencilWriteMask = 0xFFFFFFFF;
    depthStencilState->depthBias = 0;
    depthStencilState->depthBiasSlopeScale = 0;
    depthStencilState->depthBiasClamp = 0;
    stencil_set_default(&depthStencilState->stencilFront);
    stencil_set_default(&depthStencilState->stencilBack);
}

b8 webgpu_pipeline_create(WEBGPU_CONTEXT* context, WGPUVertexBufferLayout vertex_buffer_layout, WEBGPU_MATERIAL_SHADER* shader){

    
    // [...] Describe render pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.nextInChain = NULL;
    // [...] Describe vertex pipeline state
    pipelineDesc.label = (WGPUStringView){ "Object shader pipeline", sizeof("Object shader pipeline") };

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertex_buffer_layout;
    pipelineDesc.vertex.module = shader->shader_module;
    pipelineDesc.vertex.entryPoint = (WGPUStringView){ "vs_main",7};

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
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CW;
    
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;
    // [...] Describe fragment pipeline state
    // We tell that the programmable fragment shader stage is described
    // by the function called 'fs_main' in the shader module.
    WGPUFragmentState fragmentState = {};
    fragmentState.module = shader->shader_module;
    fragmentState.entryPoint = (WGPUStringView){ "fs_main",7};
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
    WGPUDepthStencilState depthStencilState;
    depth_set_default(&depthStencilState);
    depthStencilState.nextInChain = NULL;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = WGPUTextureFormat_Depth24Plus;
    // Deactivate the stencil alltogether
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;

    // Setup depth state
    pipelineDesc.depthStencil = &depthStencilState;
    // [...] Describe multi-sampling state
    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    pipelineDesc.layout = shader->pipeline.layout;
    shader->pipeline.handle = wgpuDeviceCreateRenderPipeline(context->device, &pipelineDesc);

    
    wgpuShaderModuleRelease(shader->shader_module);

    return true;
}

void webgpu_pipeline_destroy(WEBGPU_CONTEXT* context, WEBGPU_MATERIAL_SHADER* shader)
{
    wgpuRenderPipelineRelease(shader->pipeline.handle);
}