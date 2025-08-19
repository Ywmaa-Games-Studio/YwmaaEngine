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

i32 yrandom(void) {
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

f32 fyrandom(void) {
    return (float)yrandom() / (f32)RAND_MAX;
}

f32 fyrandom_in_range(f32 min, f32 max) {
    return min + ((float)yrandom() / ((f32)RAND_MAX / (max - min)));
}

Plane3D plane_3d_create(Vector3 p1, Vector3 norm) {
    Plane3D p;
    p.normal = Vector3_normalized(norm);
    p.distance = Vector3_dot(p.normal, p1);
    return p;
}

Frustum frustum_create(const Vector3* position, const Vector3* forward, const Vector3* right, const Vector3* up, f32 aspect, f32 fov, f32 near, f32 far) {
    Frustum f;

    f32 half_v = far * tanf(fov * 0.5f);
    f32 half_h = half_v * aspect;
    Vector3 fwd = *forward;
    Vector3 forward_far = Vector3_multiply_scalar(fwd, far);

    // Top, bottom, right, left, far, near
    f.sides[0] = plane_3d_create(Vector3_add(Vector3_multiply_scalar(fwd, near), *position), fwd);
    f.sides[1] = plane_3d_create(Vector3_add(*position, forward_far), Vector3_multiply_scalar(fwd, -1.0f));
    f.sides[2] = plane_3d_create(*position, Vector3_cross(*up, Vector3_add(forward_far, Vector3_multiply_scalar(*right, half_h))));
    f.sides[3] = plane_3d_create(*position, Vector3_cross(Vector3_sub(forward_far, Vector3_multiply_scalar(*right, half_h)), *up));
    f.sides[4] = plane_3d_create(*position, Vector3_cross(*right, Vector3_sub(forward_far, Vector3_multiply_scalar(*up, half_v))));
    f.sides[5] = plane_3d_create(*position, Vector3_cross(Vector3_add(forward_far, Vector3_multiply_scalar(*up, half_v)), *right));

    return f;
}

f32 plane_signed_distance(const Plane3D* p, const Vector3* position) {
    return Vector3_dot(p->normal, *position) - p->distance;
}

b8 plane_intersects_sphere(const Plane3D* p, const Vector3* center, f32 radius) {
    return plane_signed_distance(p, center) > -radius;
}

b8 frustum_intersects_sphere(const Frustum* f, const Vector3* center, f32 radius) {
    for (u8 i = 0; i < 6; ++i) {
        if (!plane_intersects_sphere(&f->sides[i], center, radius)) {
            return false;
        }
    }
    return true;
}

b8 plane_intersects_aabb(const Plane3D* p, const Vector3* center, const Vector3* extents) {
    f32 r = extents->x * yabs(p->normal.x) +
            extents->y * yabs(p->normal.y) +
            extents->z * yabs(p->normal.z);

    return -r <= plane_signed_distance(p, center);
}

b8 frustum_intersects_aabb(const Frustum* f, const Vector3* center, const Vector3* extents) {
    for (u8 i = 0; i < 6; ++i) {
        if (!plane_intersects_aabb(&f->sides[i], center, extents)) {
            return false;
        }
    }
    return true;
}
