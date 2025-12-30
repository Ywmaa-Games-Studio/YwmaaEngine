#pragma once

#include "defines.h"

#include "math/math_types.h"

typedef struct DIRECTIONAL_LIGHT {
    Vector4 colour;
    Vector4 direction;  // ignore w
} DIRECTIONAL_LIGHT;

typedef struct POINT_LIGHT {
    Vector4 colour;
    Vector4 position;  // ignore w
    // Usually 1, make sure denominator never gets smaller than 1
    f32 constant_f;
    // Reduces light intensity linearly
    f32 linear;
    // Makes the light fall off slower at longer distances.
    f32 quadratic;
    f32 padding;
} POINT_LIGHT;

b8 light_system_init(u64* memory_requirement, void* memory, void* config);
void light_system_shutdown(void* state);

YAPI b8 light_system_add_directional(DIRECTIONAL_LIGHT* light);
YAPI b8 light_system_add_point(POINT_LIGHT* light);

YAPI b8 light_system_remove_directional(DIRECTIONAL_LIGHT* light);
YAPI b8 light_system_remove_point(POINT_LIGHT* light);

YAPI DIRECTIONAL_LIGHT* light_system_directional_light_get(void);

YAPI i32 light_system_point_light_count(void);
YAPI b8 light_system_point_lights_get(POINT_LIGHT* p_lights);
