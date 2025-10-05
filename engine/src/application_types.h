#pragma once

#include "core/engine.h"
#include "memory/linear_allocator.h"

struct RENDER_PACKET;

typedef struct APP_FRAME_DATA {
    // A darray of world geometries to be rendered this frame.
    GEOMETRY_RENDER_DATA* world_geometries;
} APP_FRAME_DATA;

/**
 * @brief Represents the basic application state in a application.
 * Called for creation by the application.
 */
typedef struct APPLICATION {
    /** @brief The APPLICATION configuration. */
    APPLICATION_CONFIG app_config;

    /**
     * @brief Function pointer to the APPLICATION's boot sequence. This should 
     * fill out the APPLICATION config with the APPLICATION's specific requirements.
     * @param app_instance A pointer to the APPLICATION instance.
     * @returns True on success; otherwise false.
     */
    b8 (*boot)(struct APPLICATION* app_instance);

    /** 
     * @brief Function pointer to APPLICATION's initialize function. 
     * @param app_instance A pointer to the APPLICATION instance.
     * @returns True on success; otherwise false.
     * */
    b8 (*init)(struct APPLICATION* app_instance);

    /** 
     * @brief Function pointer to APPLICATION's update function. 
     * @param app_instance A pointer to the APPLICATION instance.
     * @param delta_time The time in seconds since the last frame.
     * @returns True on success; otherwise false.
     * */
    b8 (*update)(struct APPLICATION* app_instance, f32 delta_time);

    /** 
     * @brief Function pointer to APPLICATION's render function. 
     * @param app_instance A pointer to the APPLICATION instance.
     * @param packet A pointer to the packet to be populated by the APPLICATION.
     * @param delta_time The time in seconds since the last frame.
     * @returns True on success; otherwise false.
     * */
    b8 (*render)(struct APPLICATION* app_instance, struct RENDER_PACKET* packet, f32 delta_time);

    /** 
     * @brief Function pointer to handle resizes, if applicable. 
     * @param app_instance A pointer to the APPLICATION instance.
     * @param width The width of the window in pixels.
     * @param height The height of the window in pixels.
     * */
    void (*on_resize)(struct APPLICATION* app_instance, u32 width, u32 height);

    /**
     * @brief Shuts down the APPLICATION, prompting release of resources.
     * @param app_instance A pointer to the APPLICATION instance.
     */
    void (*shutdown)(struct APPLICATION* app_instance);

    /** @brief The required size for the APPLICATION state. */
    u64 state_memory_requirement;

    /** @brief APPLICATION-specific state. Created and managed by the APPLICATION. */
    void* state;

    /** @brief A block of memory to hold the engine state. Created and managed by the engine. */
    void* engine_state;

    /** 
     * @brief An allocator used for allocations needing to be made every frame. Contents are wiped
     * at the beginning of the frame.
     */
    LINEAR_ALLOCATOR frame_allocator;

    /** @brief Data which is built up, used and discarded every frame. */
    APP_FRAME_DATA frame_data;
} APPLICATION;
