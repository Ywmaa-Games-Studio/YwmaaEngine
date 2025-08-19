#pragma once

#include "core/application.h"
#include "memory/linear_allocator.h"

struct RENDER_PACKET;
typedef struct GAME_FRAME_DATA {
    // A darray of world geometries to be rendered this frame.
    GEOMETRY_RENDER_DATA* world_geometries;
} GAME_FRAME_DATA;
/**
 * Represents the basic game state in a game.
 * Called for creation by the application.
 */
typedef struct GAME {
    // The application configuration.
    APPLICATION_CONFIG app_config;

    /**
     * @brief Function pointer to the game's boot sequence. This should 
     * fill out the application config with the game's specific requirements.
     * @param game_instance A pointer to the game instance.
     * @returns True on success; otherwise false.
     */
    b8 (*boot)(struct GAME* game_instance);

    // Function pointer to game's initialize function.
    b8 (*init)(struct GAME* game_instance);

    // Function pointer to game's update function.
    b8 (*update)(struct GAME* game_instance, f32 delta_time);

    // Function pointer to game's render function.
    b8 (*render)(struct GAME* game_instance, struct RENDER_PACKET* packet, f32 delta_time);

    // Function pointer to handle resizes, if applicable.
    void (*on_resize)(struct GAME* game_instance, u32 width, u32 height);

    /**
     * @brief Shuts down the game, prompting release of resources.
     * @param game_instance A pointer to the game instance.
     */
    void (*shutdown)(struct GAME* game_instance);

    /** @brief The required size for the game state. */
    u64 state_memory_requirement;

    // Game-specific game state. Created and managed by the game.
    void* state;

    // Application state.
    void* application_state;

    /** 
     * @brief An allocator used for allocations needing to be made every frame. Contents are wiped
     * at the beginning of the frame.
     */
    LINEAR_ALLOCATOR frame_allocator;

    /** @brief Data which is built up, used and discarded every frame. */
    GAME_FRAME_DATA frame_data;
} GAME;
