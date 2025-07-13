#include "geometry_utils.h"

#include "ymath.h"
#include "core/logger.h"

void geometry_generate_normals(u32 vertex_count, Vertex3D* vertices, u32 index_count, u32* indices) {
    for (u32 i = 0; i < index_count; i += 3) {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        Vector3 edge1 = Vector3_sub(vertices[i1].position, vertices[i0].position);
        Vector3 edge2 = Vector3_sub(vertices[i2].position, vertices[i0].position);

        Vector3 normal = Vector3_normalized(Vector3_cross(edge1, edge2));

        // NOTE: This just generates a face normal. Smoothing out should be done in a separate pass if desired.
        vertices[i0].normal = normal;
        vertices[i1].normal = normal;
        vertices[i2].normal = normal;
    }
}

void geometry_generate_tangents(u32 vertex_count, Vertex3D* vertices, u32 index_count, u32* indices) {
    for (u32 i = 0; i < index_count; i += 3) {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        Vector3 edge1 = Vector3_sub(vertices[i1].position, vertices[i0].position);
        Vector3 edge2 = Vector3_sub(vertices[i2].position, vertices[i0].position);

        f32 deltaU1 = vertices[i1].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV1 = vertices[i1].texcoord.y - vertices[i0].texcoord.y;

        f32 deltaU2 = vertices[i2].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV2 = vertices[i2].texcoord.y - vertices[i0].texcoord.y;

        f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
        f32 fc = 1.0f / dividend;

        Vector3 tangent = (Vector3){
            (fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
            (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
            (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z))};

        tangent = Vector3_normalized(tangent);

        f32 sx = deltaU1, sy = deltaU2;
        f32 tx = deltaV1, ty = deltaV2;
        f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

        Vector3 t4 = Vector3_mul_scalar(tangent, handedness);
        vertices[i0].tangent = t4;
        vertices[i1].tangent = t4;
        vertices[i2].tangent = t4;
    }
}

b8 vertex3d_equal(Vertex3D vert_0, Vertex3D vert_1) {
    return Vector3_compare(vert_0.position, vert_1.position, Y_FLOAT_EPSILON) &&
           Vector3_compare(vert_0.normal, vert_1.normal, Y_FLOAT_EPSILON) &&
           Vector2_compare(vert_0.texcoord, vert_1.texcoord, Y_FLOAT_EPSILON) &&
           Vector4_compare(vert_0.color, vert_1.color, Y_FLOAT_EPSILON) &&
           Vector3_compare(vert_0.tangent, vert_1.tangent, Y_FLOAT_EPSILON);
}

void reassign_index(u32 index_count, u32* indices, u32 from, u32 to) {
    for (u32 i = 0; i < index_count; ++i) {
        if (indices[i] == from) {
            indices[i] = to;
        } else if (indices[i] > from) {
            // Pull in all indicies higher than 'from' by 1.
            indices[i]--;
        }
    }
}

void geometry_deduplicate_vertices(u32 vertex_count, Vertex3D* vertices, u32 index_count, u32* indices, u32* out_vertex_count, Vertex3D** out_vertices) {
    // Create new arrays for the collection to sit in.
    if (vertex_count == 0 || vertices == 0 || index_count == 0 || indices == 0) {
        PRINT_WARNING("geometry_deduplicate_vertices: Invalid input parameters. No vertices or indices provided.");
        *out_vertex_count = 0;
        *out_vertices = 0;
        return;
    }
    Vertex3D* unique_verts = yallocate(sizeof(Vertex3D) * vertex_count, MEMORY_TAG_ARRAY);
    *out_vertex_count = 0;

    u32 found_count = 0;
    for (u32 v = 0; v < vertex_count; ++v) {
        b8 found = false;
        for (u32 u = 0; u < *out_vertex_count; ++u) {
            if (vertex3d_equal(vertices[v], unique_verts[u])) {
                // Reassign indices, do _not_ copy
                reassign_index(index_count, indices, v - found_count, u);
                found = true;
                found_count++;
                break;
            }
        }

        if (!found) {
            // Copy over to unique.
            unique_verts[*out_vertex_count] = vertices[v];
            (*out_vertex_count)++;
        }
    }
    if (*out_vertex_count == 0) {
        PRINT_WARNING("geometry_deduplicate_vertices: No unique vertices found. This is unexpected.");
        yfree(unique_verts);
        *out_vertices = 0;
        return;
    }
    // Allocate new vertices array
    if (*out_vertex_count > vertex_count) {
        PRINT_WARNING("geometry_deduplicate_vertices: Unique vertex count (%d) exceeds original vertex count (%d). This is unexpected.", *out_vertex_count, vertex_count);
        yfree(unique_verts);
        *out_vertices = 0;
        return;
    }
    *out_vertices = yallocate(sizeof(Vertex3D) * (*out_vertex_count), MEMORY_TAG_ARRAY);
    // Copy over unique
    ycopy_memory(*out_vertices, unique_verts, sizeof(Vertex3D) * (*out_vertex_count));
    // Destroy temp array
    yfree(unique_verts);

    u32 removed_count = vertex_count - *out_vertex_count;
    PRINT_DEBUG("geometry_deduplicate_vertices: removed %d vertices, orig/now %d/%d.", removed_count, vertex_count, *out_vertex_count);
}
