#pragma once

#include "defines.h"

#include "resources/resource_types.h"

#define DEFAULT_MATERIAL_NAME "default"

typedef struct MATERIAL_SYSTEM_CONFIG {
    u32 max_material_count;
} MATERIAL_SYSTEM_CONFIG;

b8 material_system_init(u64* memory_requirement, void* state, void* config);
void material_system_shutdown(void* state);

YAPI MATERIAL* material_system_acquire(const char* name);
YAPI MATERIAL* material_system_acquire_from_config(MATERIAL_CONFIG config);
YAPI void material_system_release(const char* name);

YAPI MATERIAL* material_system_get_default(void);

/**
 * @brief Applies global-level data for the material shader id.
 * 
 * @param shader_id The identifier of the shader to apply globals for.
 * @param renderer_frame_number The renderer's current frame number.
 * @param projection A constant pointer to a projection matrix.
 * @param view A constant pointer to a view matrix.
 * @param view_position The camera position.
 * @param render_mode The render mode.
 * @return True on success; otherwise false.
 */
YAPI b8 material_system_apply_global(u32 shader_id, u64 renderer_frame_number, const Matrice4* projection, const Matrice4* view, const Vector4* ambient_color, const Vector3* view_position, u32 render_mode);

/**
 * @brief Applies instance-level material data for the given material.
 *
 * @param m A pointer to the material to be applied.
 * @param needs_update Indicates if material internals require updating, or if they should just be bound.
 * @return True on success; otherwise false.
 */
YAPI b8 material_system_apply_instance(MATERIAL* m, b8 needs_update);

/**
 * @brief Applies local-level material data (typically just model matrix).
 *
 * @param m A pointer to the material to be applied.
 * @param model A constant pointer to the model matrix to be applied.
 * @return True on success; otherwise false.
 */
YAPI b8 material_system_apply_local(MATERIAL* m, const Matrice4* model);
