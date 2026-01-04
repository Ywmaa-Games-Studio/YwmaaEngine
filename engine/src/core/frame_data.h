#pragma once

#include "defines.h"

struct LINEAR_ALLOCATOR;

/**
 * @brief Engine-level current frame-specific data.
 */
typedef struct FRAME_DATA {
    /** @brief The time in seconds since the last frame. */
    f32 delta_time;

    /** @brief The total amount of time in seconds the application has been running. */
    f64 total_time;

    /** @brief The number of meshes drawn in the last frame. */
    u32 drawn_mesh_count;

    /** @brief A pointer to the engine's frame allocator. */
    struct LINEAR_ALLOCATOR* frame_allocator;

    /** @brief Application level frame specific data. Optional, up to the app to know how to use this if needed. */
    void* application_frame_data;
} FRAME_DATA;
