/**
 * @file systems_manager.h
 * @brief Contains logic for the management of various engine systems, which
 * are in turn registered with this manager whose lifecycle is then automatically
 * managed thereafter.
 * 
 */
#pragma once

#include "defines.h"
#include "memory/linear_allocator.h"

/** @brief Typedef for a system initialize function pointer. */
typedef b8 (*PFN_system_init)(u64* memory_requirement, void* memory, void* config);
/** @brief Typedef for a system shutdown function pointer. */
typedef void (*PFN_system_shutdown)(void* state);
/** @brief Typedef for a update initialize function pointer. */
typedef b8 (*PFN_system_update)(void* state, f32 delta_time);

/**
 * @brief Represents a registered system. Function pointers 
 * for init, shutdown and (optionally) update are held here,
 * as well as state for the system.
 */
typedef struct Y_SYSTEM {
    /** @brief The size of the state for the system. */
    u64 state_size;
    /** @brief The state for the system. */
    void* state;
    /** @brief A function pointer for the initialization routine. Required. */
    PFN_system_init initialize;
    /** @brief A function pointer for the shutdown routine. Required. */
    PFN_system_shutdown shutdown;
    /** @brief A function pointer for the system's update routine, called every frame. Optional. */
    PFN_system_update update;
} Y_SYSTEM;

#define Y_SYSTEM_TYPE_MAX_COUNT 512

/**
 * @brief Represents the known system types within
 * the engine core up to Y_SYSTEM_TYPE_KNOWN_MAX.
 * User enumerations can start off at Y_SYSTEM_TYPE_KNOWN_MAX + 1
 * to register their systems.
 */
typedef enum E_Y_SYSTEM_TYPE {
    Y_SYSTEM_TYPE_MEMORY = 0,
    Y_SYSTEM_TYPE_CONSOLE,
    Y_SYSTEM_TYPE_YVAR,
    Y_SYSTEM_TYPE_EVENT,
    Y_SYSTEM_TYPE_LOGGING,
    Y_SYSTEM_TYPE_INPUT,
    Y_SYSTEM_TYPE_PLATFORM,
    Y_SYSTEM_TYPE_RESOURCE,
    Y_SYSTEM_TYPE_SHADER,
    Y_SYSTEM_TYPE_JOB,
    Y_SYSTEM_TYPE_TEXTURE,
    Y_SYSTEM_TYPE_FONT,
    Y_SYSTEM_TYPE_CAMERA,
    Y_SYSTEM_TYPE_RENDERER,
    Y_SYSTEM_TYPE_RENDER_VIEW,
    Y_SYSTEM_TYPE_MATERIAL,
    Y_SYSTEM_TYPE_GEOMETRY,

    // NOTE: Anything beyond this is in user space.
    Y_SYSTEM_TYPE_KNOWN_MAX = 255,

    // The user-space max
    Y_SYSTEM_TYPE_USER_MAX = Y_SYSTEM_TYPE_MAX_COUNT,
    // The max, including all user-space types.
    Y_SYSTEM_TYPE_MAX = Y_SYSTEM_TYPE_USER_MAX
} E_Y_SYSTEM_TYPE;

/**
 * @brief The state for the systems manager. Holds the
 * allocator used for all systems as well as the instances
 * and states of the registered systems themselves.
 */
typedef struct SYSTEMS_MANAGER_STATE {
    /** @brief The allocator used to obtain state memory for registered systems.*/
    LINEAR_ALLOCATOR systems_allocator;
    /** @brief The registered systems array. */
    Y_SYSTEM systems[Y_SYSTEM_TYPE_MAX_COUNT];
} SYSTEMS_MANAGER_STATE;

struct APPLICATION_CONFIG;

/**
 * @brief Initializes the system manager for all systems which must be setup
 * before the application boot sequence (i.e. events, renderer, etc.).
 * 
 * @param state A pointer to the system manager state to be initialized.
 * @param app_config A pointer to the application configuration.
 * @return b8 True if successful; otherwise false.
 */
b8 systems_manager_init(SYSTEMS_MANAGER_STATE* state, struct APPLICATION_CONFIG* app_config);
/**
 * @brief Initializes the system manager for all systems which must be setup
 * after the application boot sequence (i.e. that require the application to configure them, such as fonts, etc.).
 * 
 * @param state A pointer to the system manager state to be initialized.
 * @param app_config A pointer to the application configuration.
 * @return b8 True if successful; otherwise false.
 */
b8 systems_manager_post_boot_init(SYSTEMS_MANAGER_STATE* state, struct APPLICATION_CONFIG* app_config);
/**
 * @brief Shuts the systems manager down.
 * 
 * @param state A pointer to the system manager state to be shut down.
 */
void systems_manager_shutdown(SYSTEMS_MANAGER_STATE* state);

/**
 * @brief Calls update routines on all systems that opt in to the update.
 * Performed during the main engine loop.
 * 
 * @param state A pointer to the systems manager state.
 * @param delta_time The delta time in seconds since the last frame.
 * @return b8 True on success; otherwise false.
 */
b8 systems_manager_update(SYSTEMS_MANAGER_STATE* state, u32 delta_time);

/**
 * @brief Registers a system to be managed.
 * 
 * @param state A pointer to the system manager state.
 * @param type The system type. For known types, a k_system_type. Otherwise a user type.
 * @param initialize A function pointer for the initialize routine. Required.
 * @param shutdown A function pointer for the shutdown routine. Required.
 * @param update A function pointer for the update routine. Optional.
 * @param config A pointer to the configuration for the system, passed to initialize.
 * @return True on successful registration; otherwise false.
 */
YAPI b8 systems_manager_register(
    SYSTEMS_MANAGER_STATE* state,
    u16 type,
    PFN_system_init initialize,
    PFN_system_shutdown shutdown,
    PFN_system_update update,
    void* config);
