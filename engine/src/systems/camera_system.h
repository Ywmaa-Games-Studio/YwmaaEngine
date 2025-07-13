#pragma once

#include "renderer/camera.h"

/** @brief The Camera system configuration. */
typedef struct CAMERA_SYSTEM_CONFIG {
    /**
     * @brief NOTE: The maximum number of cameras that can be managed by
     * the system.
     */
    u16 max_camera_count;

} CAMERA_SYSTEM_CONFIG;

/** @brief The name of the default Camera. */
#define DEFAULT_CAMERA_NAME "default"

/**
 * @brief Initializes the Camera system.
 * Should be called twice; once to get the memory requirement (passing state=0), and a second
 * time passing an allocated block of memory to actually initialize the system.
 *
 * @param memory_requirement A pointer to hold the memory requirement as it is calculated.
 * @param state A block of memory to hold the state or, if gathering the memory requirement, 0.
 * @param config The configuration for this system.
 * @return True on success; otherwise false.
 */
b8 camera_system_init(u64* memory_requirement, void* state, CAMERA_SYSTEM_CONFIG config);

/**
 * @brief Shuts down the geometry Camera.
 *
 * @param state The state block of memory.
 */
void camera_system_shutdown(void* state);

/**
 * @brief Acquires a pointer to a Camera by name.
 * If one is not found, a new one is created and retuned.
 * Internal reference counter is incremented.
 * 
 * @param name The name of the Camera to acquire.
 * @return A pointer to a Camera if successful; 0 if an error occurs.
 */
YAPI Camera* camera_system_acquire(const char* name);

/**
 * @brief Releases a Camera with the given name. Intenral reference
 * counter is decremented. If this reaches 0, the Camera is reset,
 * and the reference is usable by a new Camera.
 * 
 * @param name The name of the Camera to release.
 */
YAPI void camera_system_release(const char* name);

/**
 * @brief Gets a pointer to the default Camera.
 * 
 * @return A pointer to the default Camera.
 */
YAPI Camera* camera_system_get_default(void);
