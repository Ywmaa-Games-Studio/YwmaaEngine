#pragma once
#include "renderer/renderer_types.inl"
#include "defines.h"
//#include <webgpu.h>
//#include <wgpu.h>
#define WGPU_SHARED_LIBRARY
#include "../thirdparty/wgpu/include/wgpu.h"
#include "../thirdparty/wgpu/include/webgpu.h"

typedef struct WEBGPU_PIPELINE {
    WGPURenderPipeline handle;
    WGPUPipelineLayout layout;
    WGPUBindGroup bind_group;
    WGPUBindGroupLayout bind_group_layout;
} WEBGPU_PIPELINE;

// Max number of objects
#define WEBGPU_OBJECT_MAX_OBJECT_COUNT 1024
typedef struct WEBGPU_OBJECT_SHADER {
    WEBGPU_PIPELINE pipeline;
    WGPUShaderModule shader_module;

    // Global uniform object.
    GLOBAL_UNIFORM_OBJECT global_ubo;

    // Global uniform buffer.
    WGPUBufferDescriptor global_descriptor;
    WGPUBuffer global_uniform_buffer;


} WEBGPU_OBJECT_SHADER;

typedef struct WEBGPU_CONTEXT {
    f32 frame_delta_time;

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
    WGPURequiredLimits required_limits;
    WGPUSurface surface;
    WGPUQueue queue;
    WGPUTextureView target_view;
    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder render_pass;
    WGPUTextureFormat swapchain_format;

    WGPUBuffer object_vertex_buffer;
    WGPUBuffer object_index_buffer;

    WEBGPU_OBJECT_SHADER object_shader;

} WEBGPU_CONTEXT;