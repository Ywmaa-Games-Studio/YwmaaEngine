#pragma once

#include "defines.h"


#pragma pack(push, 1)  // Remove all padding
typedef struct Vector2 {
    union {
        struct { f32 x, y; };
        struct { f32 r, g; };
        struct { f32 s, t; };
        struct { f32 u, v; };
        f32 elements[2];
    };
} Vector2;

typedef struct Vector3 {
    union {
        struct { f32 x, y, z; };
        struct { f32 r, g, b; };
        struct { f32 s, t, p; };
        struct { f32 u, v, w; };
        f32 elements[3];
    };
} Vector3;

typedef struct Vector4 {
    union {
        struct { f32 x, y, z, w; };
        struct { f32 r, g, b, a; };
        struct { f32 s, t, p, q; };
        f32 elements[4];
    };
} Vector4;
#pragma pack(pop)  // Restore default packing

typedef Vector4 Quaternion;
typedef Vector4 Color;

typedef union Matrice_u {
    f32 data[16];
} Matrice4;

typedef struct Vertex3D {
    Vector3 position;
    /** @brief The normal of the vertex. */
    Vector3 normal;
    Vector2 texcoord;
    /** @brief The color of the vertex. */
    Vector4 color;
    /** @brief The tangent of the vertex. */
    Vector4 tangent;
} Vertex3D;
typedef struct Vertex2D {
    Vector2 position;
    Vector2 texcoord;
} Vertex2D;

/**
 * @brief Represents the transform of an object in the world.
 * Transforms can have a parent whose own transform is then
 * taken into account. NOTE: The properties of this should not
 * be edited directly, but done via the functions in transform.h
 * to ensure proper matrix generation.
 */
typedef struct Transform {
    /** @brief The position in the world. */
    Vector3 position;
    /** @brief The rotation in the world. */
    Quaternion rotation;
    /** @brief The scale in the world. */
    Vector3 scale;
    /**
     * @brief Indicates if the position, rotation or scale have changed,
     * indicating that the local matrix needs to be recalculated.
     */
    b8 is_dirty;
    /**
     * @brief The local transformation matrix, updated whenever
     * the position, rotation or scale have changed.
     */
    Matrice4 local;

    /** @brief A pointer to a parent transform if one is assigned. Can also be null. */
    struct Transform* parent;
} Transform;
