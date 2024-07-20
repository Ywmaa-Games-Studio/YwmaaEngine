#include "ymath.h"
#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static b8 rand_seeded = false;

/**
 * Note that these are here in order to prevent having to import the
 * entire <math.h> everywhere.
 */
f32 ysin(f32 x) {
    return sinf(x);
}

f32 ycos(f32 x) {
    return cosf(x);
}

f32 ytan(f32 x) {
    return tanf(x);
}

f32 yacos(f32 x) {
    return acosf(x);
}

f32 ysqrt(f32 x) {
    return sqrtf(x);
}

f32 yabs(f32 x) {
    return fabsf(x);
}

i32 yrandom() {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return rand();
}

i32 yrandom_in_range(i32 min, i32 max) {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return (rand() % (max - min + 1)) + min;
}

f32 fyrandom() {
    return (float)yrandom() / (f32)RAND_MAX;
}

f32 fyrandom_in_range(f32 min, f32 max) {
    return min + ((float)yrandom() / ((f32)RAND_MAX / (max - min)));
}