#pragma once

#include "math/math_types.h"
#include "resources/resource_types.h"
#include "renderer/renderer_types.inl"
#include "systems/geometry_system.h"

typedef struct SKYBOX_CONFIG {
    /** @brief The name of the cubemap to be used for the skybox. */
    const char* cubemap_name;
    GEOMETRY_CONFIG g_config;
} SKYBOX_CONFIG;
typedef struct Skybox {
    SKYBOX_CONFIG config;
    TEXTURE_MAP cubemap;
    GEOMETRY* g;
    u32 instance_id;
    /** @brief Synced to the renderer's current frame number when the material has been applied that frame. */
    u64 render_frame_number;
} Skybox;

/**
 * @brief Attempts to create a skybox using the specified parameters.
 * 
 * @param config The configuration to be used when creating the skybox.
 * @param out_skybox A pointer to hold the newly-created skybox.
 * @return True on success; otherwise false.
 */
YAPI b8 skybox_create(SKYBOX_CONFIG config, Skybox* out_skybox);

YAPI b8 skybox_init(Skybox* sb);

YAPI b8 skybox_load(Skybox* sb);

YAPI b8 skybox_unload(Skybox* sb);

/**
 * @brief Destroys the provided skybox.
 * 
 * @param sb A pointer to the skybox to be destroyed.
 */
YAPI void skybox_destroy(Skybox* sb);
