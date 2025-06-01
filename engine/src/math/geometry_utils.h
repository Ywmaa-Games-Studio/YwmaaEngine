#pragma once

#include "math_types.h"

/**
 * @brief Calculates normals for the given vertex and index data. Modifies vertices in place.
 * 
 * @param vertex_count The number of vertices.
 * @param vertices An array of vertices.
 * @param index_count The number of indices.
 * @param indices An array of vertices.
 */
void geometry_generate_normals(u32 vertex_count, Vertex3D* vertices, u32 index_count, u32* indices);

/**
 * @brief Calculates tangents for the given vertex and index data. Modifies vertices in place.
 * 
 * @param vertex_count The number of vertices.
 * @param vertices An array of vertices.
 * @param index_count The number of indices.
 * @param indices An array of vertices.
 */
void geometry_generate_tangents(u32 vertex_count, Vertex3D* vertices, u32 index_count, u32* indices);
