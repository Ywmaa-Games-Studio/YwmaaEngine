#pragma once

#include "defines.h"
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>




typedef struct WEBGPU_CONTEXT {
    // The framebuffer's current width.
    u32 framebuffer_width;

    // The framebuffer's current height.
    u32 framebuffer_height;

    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUSurface surface;
    WGPUQueue queue;
    WGPUTextureView target_view;
    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder render_pass;

} WEBGPU_CONTEXT;