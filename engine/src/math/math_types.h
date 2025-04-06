#pragma once

#include "defines.h"


typedef union Vector2_u {
    // An array of x, y
    f32 elements[2];
    struct {
        union {
            // The first element.
            f32 x, r, s, u;
        };
        union {
            // The second element.
            f32 y, g, t, v;
        };
    };
} Vector2;

#pragma pack(push, 1)  // Remove all padding
typedef struct Vector3 {
    union {
        struct { f32 x, y, z; };
        struct { f32 r, g, b; };
        struct { f32 s, t, p; };
        struct { f32 u, v, w; };
        f32 elements[3];
    };
} Vector3;
#pragma pack(pop)  // Restore default packing

typedef union Vector4_u {
    // An array of x, y, z, w
    f32 elements[4];
    struct {
        union {
            // The first element.
            f32 x, r, s;
        };
        union {
            // The second element.
            f32 y, g, t;
        };
        union {
            // The third element.
            f32 z, b, p;
        };
        union {
            // The fourth element.
            f32 w, a, q;
        };
    };
} Vector4;

typedef Vector4 Quaternion;
typedef Vector4 Color;

typedef union Matrice_u {
    f32 data[16];
} Matrice4;

typedef struct Vertex3D {
    Vector3 position;
} Vertex3D;