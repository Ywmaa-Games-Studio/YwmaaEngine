#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "renderer/renderer_types.inl"
#include "memory/freelist.h"
#include "data_structures/hashtable.h"

#include "../thirdparty/volk/volk.h"

// Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)               \
    {                                \
        YASSERT(expr == VK_SUCCESS); \
    }
typedef struct VULKAN_BUFFER {
    u64 total_size;
    VkBuffer handle;
    VkBufferUsageFlagBits usage;
    b8 is_locked;
    VkDeviceMemory memory;
    i32 memory_index;
    u32 memory_property_flags;
    /** @brief The amount of memory required for the freelist. */
    u64 freelist_memory_requirement;
    /** @brief The memory block used by the internal freelist. */
    void* freelist_block;
    /** @brief A freelist to track allocations. */
    FREELIST buffer_freelist;
    b8 has_freelist;
} VULKAN_BUFFER;
typedef struct VULKAN_SWAPCHAIN_SUPPORT_INFO {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} VULKAN_SWAPCHAIN_SUPPORT_INFO;

typedef struct VULKAN_DEVICE {
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    VULKAN_SWAPCHAIN_SUPPORT_INFO swapchain_support;
    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkCommandPool graphics_command_pool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
    b8 supports_device_local_host_visible;

    VkFormat depth_format;
    /** @brief The chosen depth format's number of channels.*/
    u8 depth_channel_count;
} VULKAN_DEVICE;

typedef struct VULKAN_IMAGE {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} VULKAN_IMAGE;

typedef enum VULKAN_RENDER_PASS_STATE {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
} VULKAN_RENDER_PASS_STATE;

typedef struct VULKAN_RENDERPASS {
    VkRenderPass handle;

    f32 depth;
    u32 stencil;

    b8 has_prev_pass;
    b8 has_next_pass;

    VULKAN_RENDER_PASS_STATE state;
} VULKAN_RENDERPASS;

typedef struct VULKAN_SWAPCHAIN {
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR handle;
    u32 image_count;
    /** @brief An array of pointers to render targets, which contain swapchain images. */
    TEXTURE** render_textures;

    /** @brief The depth texture. */
    TEXTURE* depth_texture;

    /** 
     * @brief Render targets used for on-screen rendering, one per frame. 
     * The images contained in these are created and owned by the swapchain.
     * */
    RENDER_TARGET render_targets[3];
} VULKAN_SWAPCHAIN;

typedef enum VULKAN_COMMAND_BUFFER_STATE {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} VULKAN_COMMAND_BUFFER_STATE;

typedef struct VULKAN_COMMAND_BUFFER {
    VkCommandBuffer handle;

    // Command buffer state.
    VULKAN_COMMAND_BUFFER_STATE state;
} VULKAN_COMMAND_BUFFER;
typedef struct VULKAN_SHADER_STAGE {
    VkShaderModuleCreateInfo create_info;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo shader_stage_create_info;
} VULKAN_SHADER_STAGE;

typedef struct VULKAN_PIPELINE {
    VkPipeline handle;
    VkPipelineLayout pipeline_layout;
} VULKAN_PIPELINE;
// Max number of material instances
// TODO: make configurable
#define VULKAN_MAX_MATERIAL_COUNT 1024
// Max number of simultaneously uploaded geometries
// TODO: make configurable
#define VULKAN_MAX_GEOMETRY_COUNT 4096

/**
 * @brief Internal buffer data for geometry.
 */
typedef struct VULKAN_GEOMETRY_DATA {
    u32 id;
    u32 generation;
    u32 vertex_count;
    u32 vertex_element_size;
    u64 vertex_buffer_offset;
    u32 index_count;
    u32 index_element_size;
    u64 index_buffer_offset;
} VULKAN_GEOMETRY_DATA;
/*
 * @brief Max number of UI control instances
 * @todo TODO: make configurable
 */
#define VULKAN_MAX_UI_COUNT 1024
/**
 * @brief Put some hard limits in place for the count of supported textures,
 * attributes, uniforms, etc. This is to maintain memory locality and avoid
 * dynamic allocations.
 */
/** @brief The maximum number of stages (such as vertex, fragment, compute, etc.) allowed. */
#define VULKAN_SHADER_MAX_STAGES 8
/** @brief The maximum number of textures allowed at the global level. */
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
/** @brief The maximum number of textures allowed at the instance level. */
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
/** @brief The maximum number of vertex input attributes allowed. */
#define VULKAN_SHADER_MAX_ATTRIBUTES 16
/**
 * @brief The maximum number of uniforms and samplers allowed at the
 * global, instance and local levels combined. It's probably more than
 * will ever be needed.
 */
#define VULKAN_SHADER_MAX_UNIFORMS 128

/** @brief The maximum number of bindings per descriptor set. */
#define VULKAN_SHADER_MAX_BINDINGS 2
/** @brief The maximum number of push constant ranges for a shader. */
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32

/**
 * @brief Configuration for a shader stage, such as vertex or fragment.
 */
typedef struct VULKAN_SHADER_STAGE_CONFIG {
    /** @brief The shader stage bit flag. */
    VkShaderStageFlagBits stage;
    /** @brief The shader file name. */
    char file_name[255];

} VULKAN_SHADER_STAGE_CONFIG;

/**
 * @brief The configuration for a descriptor set.
 */
typedef struct VULKAN_DESCRIPTOR_SET_CONFIG {
    /** @brief The number of bindings in this set. */
    u8 binding_count;
    /** @brief An array of binding layouts for this set. */
    VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
    /** @brief The index of the sampler binding. */
    u8 sampler_binding_index;
} VULKAN_DESCRIPTOR_SET_CONFIG;

/** @brief Internal shader configuration generated by vulkan_shader_create(). */
typedef struct VULKAN_SHADER_CONFIG {
    /** @brief The number of shader stages in this shader. */
    u8 stage_count;
    /** @brief  The configuration for every stage of this shader. */
    VULKAN_SHADER_STAGE_CONFIG stages[VULKAN_SHADER_MAX_STAGES];
    /** @brief An array of descriptor pool sizes. */
    VkDescriptorPoolSize pool_sizes[2];
    /**
     * @brief The max number of descriptor sets that can be allocated from this shader.
     * Should typically be a decently high number.
     */
    u16 max_descriptor_set_count;

    /**
     * @brief The total number of descriptor sets configured for this shader.
     * Is 1 if only using global uniforms/samplers; otherwise 2.
     */
    u8 descriptor_set_count;
    /** @brief Descriptor sets, max of 2. Index 0=global, 1=instance */
    VULKAN_DESCRIPTOR_SET_CONFIG descriptor_sets[2];

    /** @brief An array of attribute descriptions for this shader. */
    VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];

    /** @brief Face culling mode, provided by the front end. */
    E_FACE_CULL_MODE cull_mode;

} VULKAN_SHADER_CONFIG;

/**
 * @brief Represents a state for a given descriptor. This is used
 * to determine when a descriptor needs updating. There is a state
 * per frame (with a max of 3).
 */
typedef struct VULKAN_DESCRIPTOR_STATE {
    /** @brief The descriptor generation, per frame. */
    u8 generations[3];
    /** @brief The identifier, per frame. Typically used for texture ids. */
    u32 ids[3];
} VULKAN_DESCRIPTOR_STATE;

/**
 * @brief Represents the state for a descriptor set. This is used to track
 * generations and updates, potentially for optimization via skipping
 * sets which do not need updating.
 */
typedef struct VULKAN_SHADER_DESCRIPTOR_SET_STATE {
    /** @brief The descriptor sets for this instance, one per frame. */
    VkDescriptorSet descriptor_sets[3];

    /** @brief A descriptor state per descriptor, which in turn handles frames. Count is managed in shader config. */
    VULKAN_DESCRIPTOR_STATE descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
} VULKAN_SHADER_DESCRIPTOR_SET_STATE;

/**
 * @brief The instance-level state for a shader.
 */
typedef struct VULKAN_SHADER_INSTANCE_STATE {
    /** @brief The instance id. INVALID_ID if not used. */
    u32 id;
    /** @brief The offset in bytes in the instance uniform buffer. */
    u64 offset;
    /** @brief  A state for the descriptor set. */
    VULKAN_SHADER_DESCRIPTOR_SET_STATE descriptor_set_state;
    /**
     * @brief Instance texture map pointers, which are used during rendering. These
     * are set by calls to set_sampler.
     */
    TEXTURE_MAP** instance_texture_maps;
} VULKAN_SHADER_INSTANCE_STATE;

/**
 * @brief Represents a generic Vulkan shader. This uses a set of inputs
 * and parameters, as well as the shader programs contained in SPIR-V
 * files to construct a shader for use in rendering.
 */
typedef struct VULKAN_SHADER {
    /** @brief The block of memory mapped to the uniform buffer. */
    void* mapped_uniform_buffer_block;

    /** @brief The shader identifier. */
    u32 id;

    /** @brief The configuration of the shader generated by vulkan_create_shader(). */
    VULKAN_SHADER_CONFIG config;

    /** @brief A pointer to the renderpass to be used with this shader. */
    VULKAN_RENDERPASS* renderpass;

    /** @brief An array of stages (such as vertex and fragment) for this shader. Count is located in config.*/
    VULKAN_SHADER_STAGE stages[VULKAN_SHADER_MAX_STAGES];

    /** @brief The descriptor pool used for this shader. */
    VkDescriptorPool descriptor_pool;

    /** @brief Descriptor set layouts, max of 2. Index 0=global, 1=instance. */
    VkDescriptorSetLayout descriptor_set_layouts[2];
    /** @brief Global descriptor sets, one per frame. */
    VkDescriptorSet global_descriptor_sets[3];
    /** @brief The uniform buffer used by this shader. */
    VULKAN_BUFFER uniform_buffer;

    /** @brief The pipeline associated with this shader. */
    VULKAN_PIPELINE pipeline;

    /** @brief The instance states for all instances. @todo TODO: make dynamic */
    u32 instance_count;
    VULKAN_SHADER_INSTANCE_STATE instance_states[VULKAN_MAX_MATERIAL_COUNT];

    /** @brief The number of global non-sampler uniforms. */
    u8 global_uniform_count;
    /** @brief The number of global sampler uniforms. */
    u8 global_uniform_sampler_count;
    /** @brief The number of instance non-sampler uniforms. */
    u8 instance_uniform_count;
    /** @brief The number of instance sampler uniforms. */
    u8 instance_uniform_sampler_count;
    /** @brief The number of local non-sampler uniforms. */
    u8 local_uniform_count;

} VULKAN_SHADER;

#define VULKAN_MAX_REGISTERED_RENDERPASSES 31

typedef struct VULKAN_CONTEXT {
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

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VULKAN_DEVICE device;

    VULKAN_SWAPCHAIN swapchain;

    void* renderpass_table_block;
    HASHTABLE renderpass_table;

    /** @brief Registered renderpasses. */
    RENDERPASS registered_passes[VULKAN_MAX_REGISTERED_RENDERPASSES];

    VULKAN_BUFFER object_vertex_buffer;
    VULKAN_BUFFER object_index_buffer;

    // darray
    VULKAN_COMMAND_BUFFER* graphics_command_buffers;

    // darray
    VkSemaphore* image_available_semaphores;

    // darray
    VkSemaphore* queue_complete_semaphores;

    u32 in_flight_fence_count;
    VkFence in_flight_fences[2];

    // Holds pointers to fences which exist and are owned elsewhere, one per frame.
    VkFence images_in_flight[3];

    u32 image_index;
    u32 current_frame;

    b8 recreating_swapchain;

    // TODO: make dynamic
    VULKAN_GEOMETRY_DATA geometries[VULKAN_MAX_GEOMETRY_COUNT];

    /** @brief Render targets used for world rendering. @note One per frame. */
    RENDER_TARGET world_render_targets[3];

    /** @brief Indicates if multi-threading is supported by this device. */
    b8 multithreading_enabled;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);

    /**
     * @brief A pointer to a function to be called when the backend requires
     * rendertargets to be refreshed/regenerated.
     */
    void (*on_rendertarget_refresh_required)(void);
#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} VULKAN_CONTEXT;
