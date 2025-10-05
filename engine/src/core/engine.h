#pragma once

#include "defines.h"
#include "systems/font_system.h"
#include "renderer/renderer_types.inl"

struct APPLICATION;

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
 * @returns True on success; otherwise false.
 */
YAPI b8 engine_run(void);
