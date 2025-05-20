#pragma once
#include "renderer/renderer_types.inl"
#include "defines.h"
//#include <webgpu.h>
//#include <wgpu.h>
#define WGPU_SHARED_LIBRARY
#include "../thirdparty/wgpu/include/wgpu.h"
#include "../thirdparty/wgpu/include/webgpu.h"

#include "memory/freelist.h"

typedef struct WEBGPU_BIND_STATE {
    // One per frame
    u32 generations;
    u32 ids;
} WEBGPU_BIND_STATE;
typedef struct WEBGPU_MATERIAL_SHADER_INSTANCE_STATE {
    WGPUBindGroup texture_bind_group;
    WEBGPU_BIND_STATE texture_bind_state;
} WEBGPU_MATERIAL_SHADER_INSTANCE_STATE;
typedef struct WEBGPU_UI_SHADER_INSTANCE_STATE {
    WGPUBindGroup texture_bind_group;
    WEBGPU_BIND_STATE texture_bind_state;
} WEBGPU_UI_SHADER_INSTANCE_STATE;

typedef struct WEBGPU_BUFFER {
    u64 total_size;
    WGPUBuffer handle;
    WGPUBufferUsage usage;
    b8 is_locked;
    /** @brief The amount of memory required for the freelist. */
    u64 freelist_memory_requirement;
    /** @brief The memory block used by the internal freelist. */
    void* freelist_block;
    /** @brief A freelist to track allocations. */
    FREELIST buffer_freelist;
} WEBGPU_BUFFER;
typedef struct WEBGPU_IMAGE {
    WGPUTexture handle;
    WGPUTextureView view;
    u32 width;
    u32 height;
} WEBGPU_IMAGE;

typedef struct WEBGPU_TEXTURE_DATA {
    WEBGPU_IMAGE image;
    WGPUSampler sampler;
} WEBGPU_TEXTURE_DATA;


typedef struct WEBGPU_PIPELINE {
    WGPURenderPipeline handle;
    WGPUPipelineLayout layout;
} WEBGPU_PIPELINE;
#define WEBGPU_MATERIAL_SHADER_SAMPLER_COUNT 1
// Max number of material instances
// TODO: make configurable
#define WEBGPU_MAX_MATERIAL_COUNT 1024
// Max number of simultaneously uploaded geometries
// TODO: make configurable
#define WEBGPU_MAX_GEOMETRY_COUNT 4096

/**
 * @brief Internal buffer data for geometry.
 */
typedef struct WEBGPU_GEOMETRY_DATA {
    u32 id;
    u32 generation;
    u32 vertex_count;
    u32 vertex_element_size;
    u64 vertex_buffer_offset;
    u32 index_count;
    u32 index_element_size;
    u64 index_buffer_offset;
} WEBGPU_GEOMETRY_DATA;
typedef struct WEBGPU_MATERIAL_SHADER {
    WEBGPU_PIPELINE pipeline;
    WGPUShaderModule shader_module;

    // Global uniform object.
    MATERIAL_SHADER_GLOBAL_UBO global_ubo;

    // Global uniform buffer.
    WEBGPU_BUFFER global_uniform_buffer;

    // Global uniform buffer.
    WEBGPU_BUFFER model_uniform_buffer;

    // Object uniform buffer.
    WEBGPU_BUFFER object_uniform_buffer;
    u32 object_uniform_buffer_index;

    WGPUBindGroup bind_group;
    WEBGPU_BIND_STATE bind_state;
    WGPUBindGroupLayout bind_group_layout;

    E_TEXTURE_USE sampler_uses[WEBGPU_MATERIAL_SHADER_SAMPLER_COUNT];

    // TODO: make dynamic
    WGPUBindGroupLayout texture_bind_group_layout;
    WEBGPU_MATERIAL_SHADER_INSTANCE_STATE instance_states[WEBGPU_MAX_MATERIAL_COUNT];


} WEBGPU_MATERIAL_SHADER;

#define WEBGPU_UI_SHADER_SAMPLER_COUNT 1

// Max number of ui control instances
// TODO: make configurable
#define WEBGPU_MAX_UI_SHADER_COUNT 1024

typedef struct WEBGPU_UI_SHADER {
    WEBGPU_PIPELINE pipeline;
    WGPUShaderModule shader_module;

    // Global uniform object.
    UI_SHADER_GLOBAL_UBO global_ubo;

    // Global uniform buffer.
    WEBGPU_BUFFER global_uniform_buffer;

    // Global uniform buffer.
    WEBGPU_BUFFER model_uniform_buffer;

    // Object uniform buffer.
    WEBGPU_BUFFER object_uniform_buffer;
    u32 object_uniform_buffer_index;

    WGPUBindGroup bind_group;
    WEBGPU_BIND_STATE bind_state;
    WGPUBindGroupLayout bind_group_layout;

    E_TEXTURE_USE sampler_uses[WEBGPU_UI_SHADER_SAMPLER_COUNT];

    // TODO: make dynamic
    WGPUBindGroupLayout texture_bind_group_layout;
    WEBGPU_UI_SHADER_INSTANCE_STATE instance_states[WEBGPU_MAX_UI_SHADER_COUNT];


} WEBGPU_UI_SHADER;

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
    WGPUSurface surface;
    WGPUQueue queue;
    WGPUTextureView target_view;
    WGPUTextureView depth_view;
    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder world_render_pass;
    WGPURenderPassEncoder ui_render_pass;
    WGPUTextureFormat swapchain_format;

    WEBGPU_BUFFER object_vertex_buffer;
    WEBGPU_BUFFER object_index_buffer;

    WEBGPU_MATERIAL_SHADER material_shader;
    WEBGPU_UI_SHADER ui_shader;

    // TODO: make dynamic
    WEBGPU_GEOMETRY_DATA geometries[WEBGPU_MAX_GEOMETRY_COUNT];

} WEBGPU_CONTEXT;
