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
    Vector2 texcoord;
} Vertex3D;