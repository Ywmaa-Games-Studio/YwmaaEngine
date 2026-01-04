#pragma once

#include "defines.h"
#include "math/math_types.h"

struct FRAME_DATA;
struct RENDER_PACKET;
struct DIRECTIONAL_LIGHT;
struct POINT_LIGHT;
struct Mesh;
struct Skybox;
struct GEOMETRY_CONFIG;
struct Camera;
struct SIMPLE_SCENE_CONFIG;
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

typedef struct PENDING_MESH {
    struct Mesh* m;

    const char* mesh_resource_name;

    u32 geometry_config_count;
    struct GEOMETRY_CONFIG** g_configs;
} PENDING_MESH;

typedef struct SIMPLE_SCENE {
    u32 id;
    SIMPLE_SCENE_STATE state;
    b8 enabled;

    char* name;
    char* description;

    Transform scene_transform;

    // Singlular pointer to a directional light.
    struct DIRECTIONAL_LIGHT* dir_light;

    // darray of point lights.
    struct POINT_LIGHT* point_lights;

    // darray of meshes.
    struct Mesh* meshes;

    // darray of meshes to be loaded.
    PENDING_MESH* pending_meshes;

    // Singlular pointer to a skybox.
    struct Skybox* sb;

    // A pointer to the scene configuration, if provided.
    struct SIMPLE_SCENE_CONFIG* config;
} SIMPLE_SCENE;

/**
 * @brief Creates a new scene with the given config with default values.
 * No resources are allocated. Config is not yet processed.
 * 
 * @param config A pointer to the configuration. Optional.
 * @param out_scene A pointer to hold the newly created scene. Required.
 * @return True on success; otherwise false. 
 */
YAPI b8 simple_scene_create(void* config, SIMPLE_SCENE* out_scene);

/**
 * @brief Performs initialization routines on the scene, including processing
 * configuration (if provided) and scaffolding heirarchy.
 * 
 * @param scene A pointer to the scene to be initialized.
 * @return True on success; otherwise false. 
 */
YAPI b8 simple_scene_init(SIMPLE_SCENE* scene);

/**
 * @brief Performs loading routines and resource allocation on the given scene.
 * 
 * @param scene A pointer to the scene to be loaded.
 * @return True on success; otherwise false. 
 */
YAPI b8 simple_scene_load(SIMPLE_SCENE* scene);

/**
 * @brief Performs unloading routines and resource de-allocation on the given scene.
 * 
 * @param scene A pointer to the scene to be unloaded.
 * @param immediate Unload immediately instead of the next frame. NOTE: can have unintended side effects if used improperly.
 * @return True on success; otherwise false. 
 */
YAPI b8 simple_scene_unload(SIMPLE_SCENE* scene, b8 immediate);

/**
 * @brief Performs any required scene updates for the given frame.
 *
 * @param scene A pointer to the scene to be updated.
 * @param p_frame_data A constant pointer to the current frame's data.
 * @return True on success; otherwise false.
 */
YAPI b8 simple_scene_update(SIMPLE_SCENE* scene, const struct FRAME_DATA* p_frame_data);

/**
 * @brief Populate the given render packet with data from the provided scene.
 *
 * @param scene A pointer to the scene to be updated.
 * @param current_camera The current camera to use while rendering the scene.
 * @param aspect The aspect ratio.
 * @param p_frame_data A constant pointer to the current frame's data.
 * @param packet A pointer to the packet to populate.
 * @return True on success; otherwise false.
 */
YAPI b8 simple_scene_populate_render_packet(SIMPLE_SCENE* scene, struct Camera* current_camera, f32 aspect, struct FRAME_DATA* p_frame_data, struct RENDER_PACKET* packet);

YAPI b8 simple_scene_add_directional_light(SIMPLE_SCENE* scene, const char* name, struct DIRECTIONAL_LIGHT* light);

YAPI b8 simple_scene_add_point_light(SIMPLE_SCENE* scene, const char* name, struct POINT_LIGHT* light);

YAPI b8 simple_scene_add_mesh(SIMPLE_SCENE* scene, const char* name, struct Mesh* m);

YAPI b8 simple_scene_add_skybox(SIMPLE_SCENE* scene, const char* name, struct Skybox* sb);

YAPI b8 simple_scene_remove_directional_light(SIMPLE_SCENE* scene, const char* name);

YAPI b8 simple_scene_remove_point_light(SIMPLE_SCENE* scene, const char* name);

YAPI b8 simple_scene_remove_mesh(SIMPLE_SCENE* scene, const char* name);

YAPI b8 simple_scene_remove_skybox(SIMPLE_SCENE* scene, const char* name);

YAPI struct DIRECTIONAL_LIGHT* simple_scene_directional_light_get(SIMPLE_SCENE* scene, const char* name);

YAPI struct POINT_LIGHT* simple_scene_point_light_get(SIMPLE_SCENE* scene, const char* name);

YAPI struct Mesh* simple_scene_mesh_get(SIMPLE_SCENE* scene, const char* name);

YAPI struct Skybox* simple_scene_skybox_get(SIMPLE_SCENE* scene, const char* name);
