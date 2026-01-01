#pragma once

#include "defines.h"
#include "systems/font_system.h"
#include "renderer/renderer_types.inl"

struct APPLICATION;
struct FRAME_DATA;

/** 
 * @brief Represents configuration for the application. The application config
 * is fed to the engine on creation, so it knows how to configure itself internally.
 */
typedef struct APPLICATION_CONFIG {
    // Window starting position x axis, if applicable.
    i16 start_pos_x;

    // Window starting position y axis, if applicable.
    i16 start_pos_y;

    // Window starting width, if applicable.
    i16 start_width;

    // Window starting height, if applicable.
    i16 start_height;

    // The renderer backend API to use.
    E_RENDERER_BACKEND_API renderer_backend_api;

    // The engine name used in windowing, if applicable.
    char* name;

    /** @brief Configuration for the font system. */
    FONT_SYSTEM_CONFIG font_config;

    /** @brief A darray of render view configurations. */
    RENDER_VIEW_CONFIG* render_views;    

    RENDERER_PLUGIN renderer_plugin;

    /** @brief The size of the frame allocator. */
    u64 frame_allocator_size;

    /** @brief The size of the application-specific frame data. Set to 0 if not used. */
    u64 app_frame_data_size;
} APPLICATION_CONFIG;

/**
 * @brief Creates the engine, standing up the platform layer and all
 * underlying subsystems.
 * @param game_instance A pointer to the application instance associated with the engine
 * @returns True on success; otherwise false.
 */
YAPI b8 engine_create(struct APPLICATION* game_instance);

/**
 * @brief Starts the main engine loop.
 * @param game_instance A pointer to the application instance associated with the engine
 * @returns True on success; otherwise false.
 */
YAPI b8 engine_run(struct APPLICATION* game_instance);

/**
 * @brief A callback made when the event system is initialized,
 * which internally allows the engine to begin listening for events
 * required for initialization.
 */
void engine_on_event_system_initialized(void);

/**
 * @brief Obtains a constant pointer to the current frame data.
 * 
 * @param game_instance A pointer to the application instance.
 * @return A constant pointer to the current frame data.
 */
YAPI const struct FRAME_DATA* engine_frame_data_get(struct APPLICATION* game_instance);
