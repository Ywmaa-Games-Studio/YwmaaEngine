#pragma clang optimize off // Disable optimizations here because sometimes they cause damage by removing important zeroed variables
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

b8 webgpu_pipeline_create(
    WEBGPU_CONTEXT* context,
    u32 bind_group_layout_count,
    WGPUBindGroupLayout* bind_group_layouts,
    WGPUVertexState* vertex_stage,
    WGPUFragmentState* fragment_stage,
    u32 push_constant_range_count,
    range* push_constant_ranges,
    b8 is_wireframe,
    b8 depth_test_enabled,
    WEBGPU_PIPELINE* pipeline
    )
    {
    // Push constants
    WGPUPipelineLayoutExtras pipeline_extras;
    pipeline_extras.chain.next = NULL;
    pipeline_extras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    if (push_constant_range_count > 0) {
        if (push_constant_range_count > 32) {
            PRINT_ERROR("webgpu_graphics_pipeline_create: cannot have more than 32 push constant ranges. Passed count: %i", push_constant_range_count);
            return false;
        }

        // NOTE: 32 is the max number of ranges we can ever have, since spec only guarantees 128 bytes with 4-byte alignment.
        WGPUPushConstantRange ranges[32];
        yzero_memory(ranges, sizeof(WGPUPushConstantRange) * 32);
        for (u32 i = 0; i < push_constant_range_count; ++i) {
            ranges[i].stages = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
            ranges[i].start = push_constant_ranges[i].offset;
            ranges[i].end = push_constant_ranges[i].offset + push_constant_ranges[i].size;
        }
        pipeline_extras.pushConstantRangeCount = push_constant_range_count;
        pipeline_extras.pushConstantRanges = ranges;
    } else {
        pipeline_extras.pushConstantRangeCount = 0;
        pipeline_extras.pushConstantRanges = 0;
    }

    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layout_desc;
    layout_desc.label = (WGPUStringView){"Object shader pipeline layout", sizeof("Object shader pipeline layout")};
    layout_desc.nextInChain = &pipeline_extras.chain;
    layout_desc.bindGroupLayoutCount = bind_group_layout_count;
    layout_desc.bindGroupLayouts = bind_group_layouts;
    pipeline->layout = wgpuDeviceCreatePipelineLayout(context->device, &layout_desc);
    // [...] Describe render pipeline
    WGPURenderPipelineDescriptor pipeline_desc = {0};
    pipeline_desc.nextInChain = NULL;
    // [...] Describe vertex pipeline state
    pipeline_desc.label = (WGPUStringView){"shader pipeline", sizeof("shader pipeline")};
    pipeline_desc.vertex = *vertex_stage;
    pipeline_desc.fragment = fragment_stage;
    // [...] Describe primitive pipeline state
    // Each sequence of 3 vertices is considered as a triangle
    pipeline_desc.primitive.topology = is_wireframe ? WGPUPrimitiveTopology_LineList : WGPUPrimitiveTopology_TriangleList;
    
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipeline_desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;
    
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipeline_desc.primitive.cullMode = WGPUCullMode_None;
    
    if (depth_test_enabled){
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
        pipeline_desc.depthStencil = &depthStencilState;
    } else {
        pipeline_desc.depthStencil = NULL;
    }

    // [...] Describe multi-sampling state
    // Samples per pixel
    pipeline_desc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipeline_desc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipeline_desc.multisample.alphaToCoverageEnabled = false;

    pipeline_desc.layout = pipeline->layout;
    pipeline->handle = wgpuDeviceCreateRenderPipeline(context->device, &pipeline_desc);

    return true;
}

void webgpu_pipeline_destroy(WEBGPU_CONTEXT* context, WEBGPU_PIPELINE* pipeline)
{
    wgpuRenderPipelineRelease(pipeline->handle);
    wgpuPipelineLayoutRelease(pipeline->layout);
}
#pragma clang optimize on
