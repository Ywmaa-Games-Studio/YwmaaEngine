#pragma once

#include "core/engine.h"
#include "platform/platform.h"

struct RENDER_PACKET;
struct FRAME_DATA;

/** @brief Represents the various stages of application lifecycle. */
typedef enum E_APPLICATION_STAGE {
    /** @brief Application is in an uninitialized state. */
    APPLICATION_STAGE_UNINITIALIZED,
    /** @brief Application is currently booting up. */
    APPLICATION_STAGE_BOOTING,
    /** @brief Application completed boot process and is ready to be initialized. */
    APPLICATION_STAGE_BOOT_COMPLETE,
    /** @brief Application is currently initializing. */
    APPLICATION_STAGE_INITIALIZING,
    /** @brief Application initialization is complete. */
    APPLICATION_STAGE_INITIALIZED,
    /** @brief Application is currently running. */
    APPLICATION_STAGE_RUNNING,
    /** @brief Application is in the process of shutting down. */
    APPLICATION_STAGE_SHUTTING_DOWN
} E_APPLICATION_STAGE;

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
     * @param p_frame_data A pointer to the current frame's data.
     * @returns True on success; otherwise false.
     * */
    b8 (*update)(struct APPLICATION* app_instance, struct FRAME_DATA* p_frame_data);

    /** 
     * @brief Function pointer to APPLICATION's render function. 
     * @param app_instance A pointer to the APPLICATION instance.
     * @param packet A pointer to the packet to be populated by the APPLICATION.
     * @param p_frame_data A pointer to the current frame's data.
     * @returns True on success; otherwise false.
     * */
    b8 (*render)(struct APPLICATION* app_instance, struct RENDER_PACKET* packet, struct FRAME_DATA* p_frame_data);

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

    void (*lib_on_unload)(struct APPLICATION* game_instance);

    void (*lib_on_load)(struct APPLICATION* game_instance);

    /** @brief The application stage of execution. */
    E_APPLICATION_STAGE stage;

    /** @brief The required size for the APPLICATION state. */
    u64 state_memory_requirement;

    /** @brief APPLICATION-specific state. Created and managed by the APPLICATION. */
    void* state;

    /** @brief A block of memory to hold the engine state. Created and managed by the engine. */
    void* engine_state;

    // TODO: Move this to somewhere better...
    DYNAMIC_LIBRARY renderer_library;
    RENDERER_PLUGIN render_plugin;

    DYNAMIC_LIBRARY game_library;
} APPLICATION;
