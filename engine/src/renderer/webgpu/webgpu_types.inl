#pragma once

#include "defines.h"
//#include <webgpu.h>
//#include <wgpu.h>
#define WGPU_SHARED_LIBRARY
#include "../thirdparty/wgpu/include/wgpu.h"
#include "../thirdparty/wgpu/include/webgpu.h"



typedef struct WEBGPU_CONTEXT {
    // The framebuffer's current width.
    u32 framebuffer_width;

    // The framebuffer's current height.
    u32 framebuffer_height;

    // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation,
    // a new one should be generated.
    u64 framebuffer_size_generation;

    // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation
    // when updated.
    u64 framebuffer_size_last_generation;

    b8 recreating_swapchain;

    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUSurface surface;
    WGPUQueue queue;
    WGPUTextureView target_view;
    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder render_pass;
    WGPUTextureFormat swapchain_format;
    
    WGPURenderPipeline pipeline;
    WGPUShaderModule shaderModule;

} WEBGPU_CONTEXT;