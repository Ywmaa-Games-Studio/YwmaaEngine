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

typedef struct Vector3_u {
    union {
        // An array of x, y, z
        f32 elements[3];
        struct {
            union {
                // The first element.
                f32 x, r, s, u;
            };
            union {
                // The second element.
                f32 y, g, t, v;
            };
            union {
                // The third element.
                f32 z, b, p, w;
            };
        };
    };
} Vector3;

typedef union Vector4_u {
    // An array of x, y, z, w
    f32 elements[4];
    union {
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
    };
} Vector4;

typedef Vector4 Quaternion;
typedef Vector4 Color;

typedef union Matrice_u {
    f32 data[16];
} Matrice4;