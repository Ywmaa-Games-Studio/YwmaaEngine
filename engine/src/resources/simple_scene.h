#pragma once

#include "defines.h"

typedef enum SIMPLE_SCENE_STATE {
    /** @brief created, but nothing more. */
    SIMPLE_SCENE_STATE_UNINITIALIZED,
    /** @brief Configuration parsed, not yet loaded hierarchy setup. */
    SIMPLE_SCENE_STATE_INITIALIZED,
    /** @brief In the process of loading the hierarchy. */
    SIMPLE_SCENE_STATE_LOADING,
    /** @brief Everything is loaded, ready to play. */
    SIMPLE_SCENE_STATE_LOADED,
    /** @brief In the process of unloading, not ready to play. */
    SIMPLE_SCENE_STATE_UNLOADING,
    /** @brief Unloaded and ready to be destroyed.*/
    SIMPLE_SCENE_STATE_UNLOADED
} SIMPLE_SCENE_STATE;

typedef struct SIMPLE_SCENE {
    u32 id;
    SIMPLE_SCENE_STATE state;
    b8 enabled;
} SIMPLE_SCENE;

/**
 * @brief Creates a new scene with the given config with default values.
 * No resources are allocated. Config is not yet processed.
 * 
 * @param config A pointer to the configuration. Optional.
 * @param out_scene A pointer to hold the newly created scene. Required.
 * @return True on success; otherwise false. 
 */
b8 simple_scene_create(void* config, SIMPLE_SCENE* out_scene);

/**
 * @brief Destroys the given scene. Performs an unload first if the scene is loaded.
 * 
 * @param scene A pointer to the scene to be unloaded. Required.
 * @return True on success; otherwise false. 
 */
b8 simple_scene_destroy(SIMPLE_SCENE* scene);

/**
 * @brief Performs initialization routines on the scene, including processing
 * configuration (if provided) and scaffolding heirarchy.
 * 
 * @param scene A pointer to the scene to be initialized.
 * @return True on success; otherwise false. 
 */
b8 simple_scene_initialize(SIMPLE_SCENE* scene);

/**
 * @brief Performs loading routines and resource allocation on the given scene.
 * 
 * @param scene A pointer to the scene to be loaded.
 * @return True on success; otherwise false. 
 */
b8 simple_scene_load(SIMPLE_SCENE* scene);

/**
 * @brief Performs unloading routines and resource de-allocation on the given scene.
 * 
 * @param scene A pointer to the scene to be unloaded.
 * @return True on success; otherwise false. 
 */
b8 simple_scene_unload(SIMPLE_SCENE* scene);

b8 simple_scene_update(SIMPLE_SCENE* scene, f32 delta_time);
