#pragma once

#include "renderer/renderer_types.inl"

typedef struct GEOMETRY_SYSTEM_CONFIG {
    // Max number of geometries that can be loaded at once.
    // NOTE: Should be significantly greater than the number of static meshes because
    // the there can and will be more than one of these per mesh.
    // Take other systems into account as well
    u32 max_geometry_count;
} GEOMETRY_SYSTEM_CONFIG;

typedef struct GEOMETRY_CONFIG {
    u32 vertex_count;
    Vertex3D* vertices;
    u32 index_count;
    u32* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char material_name[MATERIAL_NAME_MAX_LENGTH];
} GEOMETRY_CONFIG;

#define DEFAULT_GEOMETRY_NAME "default"

b8 geometry_system_init(u64* memory_requirement, void* state, GEOMETRY_SYSTEM_CONFIG config);
void geometry_system_shutdown(void* state);

/**
 * @brief Acquires an existing geometry by id.
 * 
 * @param id The geometry identifier to acquire by.
 * @return A pointer to the acquired geometry or nullptr if failed.
 */
GEOMETRY* geometry_system_acquire_by_id(u32 id);

/**
 * @brief Registers and acquires a new geometry using the given config.
 * 
 * @param config The geometry configuration.
 * @param auto_release Indicates if the acquired geometry should be unloaded when its reference count reaches 0.
 * @return A pointer to the acquired geometry or nullptr if failed. 
 */
GEOMETRY* geometry_system_acquire_from_config(GEOMETRY_CONFIG config, b8 auto_release);

/**
 * @brief Releases a reference to the provided geometry.
 * 
 * @param geometry The geometry to be released.
 */
void geometry_system_release(GEOMETRY* geometry);

/**
 * @brief Obtains a pointer to the default geometry.
 * 
 * @return A pointer to the default geometry. 
 */
GEOMETRY* geometry_system_get_default();

/**
 * @brief Generates configuration for plane geometries given the provided parameters.
 * NOTE: vertex and index arrays are dynamically allocated and should be freed upon object disposal.
 * Thus, this should not be considered production code.
 * 
 * @param width The overall width of the plane. Must be non-zero.
 * @param height The overall height of the plane. Must be non-zero.
 * @param x_segment_count The number of segments along the x-axis in the plane. Must be non-zero.
 * @param y_segment_count The number of segments along the y-axis in the plane. Must be non-zero.
 * @param tile_x The number of times the texture should tile across the plane on the x-axis. Must be non-zero.
 * @param tile_y The number of times the texture should tile across the plane on the y-axis. Must be non-zero.
 * @param name The name of the generated geometry.
 * @param material_name The name of the material to be used.
 * @return A geometry configuration which can then be fed into geometry_system_acquire_from_config().
 */
GEOMETRY_CONFIG geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y, const char* name, const char* material_name);