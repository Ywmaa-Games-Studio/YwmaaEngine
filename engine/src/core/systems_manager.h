#pragma once

#include "defines.h"
#include "memory/linear_allocator.h"

typedef b8 (*PFN_system_init)(u64* memory_requirement, void* memory, void* config);
typedef void (*PFN_system_shutdown)(void* state);
typedef b8 (*PFN_system_update)(void* state, f32 delta_time);

typedef struct Y_SYSTEM {
    u64 state_size;
    void* state;
    PFN_system_init initialize;
    PFN_system_shutdown shutdown;
    /** @brief A function pointer for the system's update routine, called every frame. Optional. */
    PFN_system_update update;
} Y_SYSTEM;

#define Y_SYSTEM_TYPE_MAX_COUNT 512

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

typedef struct SYSTEMS_MANAGER_STATE {
    LINEAR_ALLOCATOR systems_allocator;
    Y_SYSTEM systems[Y_SYSTEM_TYPE_MAX_COUNT];
} SYSTEMS_MANAGER_STATE;

struct APPLICATION_CONFIG;

b8 systems_manager_init(SYSTEMS_MANAGER_STATE* state, struct APPLICATION_CONFIG* app_config);
b8 systems_manager_post_boot_init(SYSTEMS_MANAGER_STATE* state, struct APPLICATION_CONFIG* app_config);
void systems_manager_shutdown(SYSTEMS_MANAGER_STATE* state);

b8 systems_manager_update(SYSTEMS_MANAGER_STATE* state, u32 delta_time);

b8 systems_manager_register(
    SYSTEMS_MANAGER_STATE* state,
    u16 type,
    PFN_system_init initialize,
    PFN_system_shutdown shutdown,
    PFN_system_update update,
    void* config);
