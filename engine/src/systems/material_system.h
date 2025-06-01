#pragma once

#include "defines.h"

#include "resources/resource_types.h"

#define DEFAULT_MATERIAL_NAME "default"

typedef struct MATERIAL_SYSTEM_CONFIG {
    u32 max_material_count;
} MATERIAL_SYSTEM_CONFIG;

b8 material_system_init(u64* memory_requirement, void* state, MATERIAL_SYSTEM_CONFIG config);
void material_system_shutdown(void* state);

MATERIAL* material_system_acquire(const char* name);
MATERIAL* material_system_acquire_from_config(MATERIAL_CONFIG config);
void material_system_release(const char* name);

MATERIAL* material_system_get_default(void);

/**
 * @brief Applies global-level data for the material shader id.
 * 
 * @param shader_id The identifier of the shader to apply globals for.
 * @param projection A constant pointer to a projection matrix.
 * @param view A constant pointer to a view matrix.
 * @param view_position The camera position.
 * @return True on success; otherwise false.
 */
b8 material_system_apply_global(u32 shader_id, const Matrice4* projection, const Matrice4* view, const Vector4* ambient_colour, const Vector3* view_position);

/**
 * @brief Applies instance-level material data for the given material.
 *
 * @param m A pointer to the material to be applied.
 * @return True on success; otherwise false.
 */
b8 material_system_apply_instance(MATERIAL* m);

/**
 * @brief Applies local-level material data (typically just model matrix).
 *
 * @param m A pointer to the material to be applied.
 * @param model A constant pointer to the model matrix to be applied.
 * @return True on success; otherwise false.
 */
b8 material_system_apply_local(MATERIAL* m, const Matrice4* model);
