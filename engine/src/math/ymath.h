#pragma once

#include "defines.h"
#include "math_types.h"

#include "core/ymemory.h"

#define Y_PI 3.14159265358979323846f
#define Y_PI_2 2.0f * Y_PI
#define Y_HALF_PI 0.5f * Y_PI
#define Y_QUARTER_PI 0.25f * Y_PI
#define Y_ONE_OVER_PI 1.0f / Y_PI
#define Y_ONE_OVER_TWO_PI 1.0f / Y_PI_2
#define Y_SQRT_TWO 1.41421356237309504880f
#define Y_SQRT_THREE 1.73205080756887729352f
#define Y_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define Y_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define Y_DEG2RAD_MULTIPLIER Y_PI / 180.0f
#define Y_RAD2DEG_MULTIPLIER 180.0f / Y_PI

// The multiplier to convert seconds to milliseconds.
#define Y_SEC_TO_MS_MULTIPLIER 1000.0f

// The multiplier to convert milliseconds to seconds.
#define Y_MS_TO_SEC_MULTIPLIER 0.001f

// A huge number that should be larger than any valid number used.
#define Y_INFINITY 1e30f

// Smallest positive number where 1.0 + FLOAT_EPSILON != 0
#define Y_FLOAT_EPSILON 1.192092896e-07f

// ------------------------------------------
// General math functions
// ------------------------------------------
YAPI f32 ysin(f32 x);
YAPI f32 ycos(f32 x);
YAPI f32 ytan(f32 x);
YAPI f32 yacos(f32 x);
YAPI f32 ysqrt(f32 x);
YAPI f32 yabs(f32 x);

/**
 * Indicates if the value is a power of 2. 0 is considered _not_ a power of 2.
 * @param value The value to be interpreted.
 * @returns True if a power of 2, otherwise false.
 */
YINLINE b8 is_power_of_2(u64 value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

YAPI i32 yrandom();
YAPI i32 yrandom_in_range(i32 min, i32 max);

YAPI f32 fyrandom();
YAPI f32 fyrandom_in_range(f32 min, f32 max);

// ------------------------------------------
// Vector 2
// ------------------------------------------

/**
 * @brief Creates and returns a new 2-element vector using the supplied values.
 * 
 * @param x The x value.
 * @param y The y value.
 * @return A new 2-element vector.
 */
YINLINE Vector2 Vector2_create(f32 x, f32 y) {
    Vector2 out_vector;
    out_vector.x = x;
    out_vector.y = y;
    return out_vector;
}

/**
 * @brief Creates and returns a 2-component vector with all components set to 0.0f.
 */
YINLINE Vector2 Vector2_zero() {
    return (Vector2){0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 2-component vector with all components set to 1.0f.
 */
YINLINE Vector2 Vector2_one() {
    return (Vector2){1.0f, 1.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing up (0, 1).
 */
YINLINE Vector2 Vector2_up() {
    return (Vector2){0.0f, 1.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing down (0, -1).
 */
YINLINE Vector2 Vector2_down() {
    return (Vector2){0.0f, -1.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing left (-1, 0).
 */
YINLINE Vector2 Vector2_left() {
    return (Vector2){-1.0f, 0.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing right (1, 0).
 */
YINLINE Vector2 Vector2_right() {
    return (Vector2){1.0f, 0.0f};
}

/**
 * @brief Adds vector_1 to vector_0 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector2 Vector2_add(Vector2 vector_0, Vector2 vector_1) {
    return (Vector2){
        vector_0.x + vector_1.x,
        vector_0.y + vector_1.y};
}

/**
 * @brief Subtracts vector_1 from vector_0 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector2 Vector2_sub(Vector2 vector_0, Vector2 vector_1) {
    return (Vector2){
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y};
}

/**
 * @brief Multiplies vector_0 by vector_1 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector2 Vector2_mul(Vector2 vector_0, Vector2 vector_1) {
    return (Vector2){
        vector_0.x * vector_1.x,
        vector_0.y * vector_1.y};
}

/**
 * @brief Divides vector_0 by vector_1 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector2 Vector2_div(Vector2 vector_0, Vector2 vector_1) {
    return (Vector2){
        vector_0.x / vector_1.x,
        vector_0.y / vector_1.y};
}

/**
 * @brief Returns the squared length of the provided vector.
 * 
 * @param vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
YINLINE f32 Vector2_length_squared(Vector2 vector) {
    return vector.x * vector.x + vector.y * vector.y;
}

/**
 * @brief Returns the length of the provided vector.
 * 
 * @param vector The vector to retrieve the length of.
 * @return The length.
 */
YINLINE f32 Vector2_length(Vector2 vector) {
    return ysqrt(Vector2_length_squared(vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 * 
 * @param vector A pointer to the vector to be normalized.
 */
YINLINE void Vector2_normalize(Vector2* vector) {
    const f32 length = Vector2_length(*vector);
    vector->x /= length;
    vector->y /= length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 * 
 * @param vector The vector to be normalized.
 * @return A normalized copy of the supplied vector 
 */
YINLINE Vector2 Vector2_normalized(Vector2 vector) {
    Vector2_normalize(&vector);
    return vector;
}

/**
 * @brief Compares all elements of vector_0 and vector_1 and ensures the difference
 * is less than tolerance.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @param tolerance The difference tolerance. Typically Y_FLOAT_EPSILON or similar.
 * @return True if within tolerance; otherwise false. 
 */
YINLINE b8 Vector2_compare(Vector2 vector_0, Vector2 vector_1, f32 tolerance) {
    if (yabs(vector_0.x - vector_1.x) > tolerance) {
        return false;
    }

    if (yabs(vector_0.y - vector_1.y) > tolerance) {
        return false;
    }

    return true;
}

/**
 * @brief Returns the distance between vector_0 and vector_1.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The distance between vector_0 and vector_1.
 */
YINLINE f32 Vector2_distance(Vector2 vector_0, Vector2 vector_1) {
    Vector2 d = (Vector2){
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y};
    return Vector2_length(d);
}

// ------------------------------------------
// Vector 3
// ------------------------------------------

/**
 * @brief Creates and returns a new 3-element vector using the supplied values.
 * 
 * @param x The x value.
 * @param y The y value.
 * @param z The z value.
 * @return A new 3-element vector.
 */
YINLINE Vector3 Vector3_create(f32 x, f32 y, f32 z) {
    return (Vector3){x, y, z};
}

/**
 * @brief Returns a new Vector3 containing the x, y and z components of the 
 * supplied Vector4, essentially dropping the w component.
 * 
 * @param vector The 4-component vector to extract from.
 * @return A new Vector3 
 */
YINLINE Vector3 Vector3_from_Vector4(Vector4 vector) {
    return (Vector3){vector.x, vector.y, vector.z};
}

/**
 * @brief Returns a new Vector4 using vector as the x, y and z components and w for w.
 * 
 * @param vector The 3-component vector.
 * @param w The w component.
 * @return A new Vector4 
 */
YINLINE Vector4 Vector3_to_Vector4(Vector3 vector, f32 w) {
    return (Vector4){vector.x, vector.y, vector.z, w};
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 0.0f.
 */
YINLINE Vector3 Vector3_zero() {
    return (Vector3){0.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 1.0f.
 */
YINLINE Vector3 Vector3_one() {
    return (Vector3){1.0f, 1.0f, 1.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing up (0, 1, 0).
 */
YINLINE Vector3 Vector3_up() {
    return (Vector3){0.0f, 1.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing down (0, -1, 0).
 */
YINLINE Vector3 Vector3_down() {
    return (Vector3){0.0f, -1.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing left (-1, 0, 0).
 */
YINLINE Vector3 Vector3_left() {
    return (Vector3){-1.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing right (1, 0, 0).
 */
YINLINE Vector3 Vector3_right() {
    return (Vector3){1.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing forward (0, 0, -1).
 */
YINLINE Vector3 Vector3_forward() {
    return (Vector3){0.0f, 0.0f, -1.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing backward (0, 0, 1).
 */
YINLINE Vector3 Vector3_back() {
    return (Vector3){0.0f, 0.0f, 1.0f};
}

/**
 * @brief Adds vector_1 to vector_0 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector3 Vector3_add(Vector3 vector_0, Vector3 vector_1) {
    return (Vector3){
        vector_0.x + vector_1.x,
        vector_0.y + vector_1.y,
        vector_0.z + vector_1.z};
}

/**
 * @brief Subtracts vector_1 from vector_0 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector3 Vector3_sub(Vector3 vector_0, Vector3 vector_1) {
    return (Vector3){
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y,
        vector_0.z - vector_1.z};
}

/**
 * @brief Multiplies vector_0 by vector_1 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector3 Vector3_mul(Vector3 vector_0, Vector3 vector_1) {
    return (Vector3){
        vector_0.x * vector_1.x,
        vector_0.y * vector_1.y,
        vector_0.z * vector_1.z};
}

/**
 * @brief Multiplies all elements of vector_0 by scalar and returns a copy of the result.
 * 
 * @param vector_0 The vector to be multiplied.
 * @param scalar The scalar value.
 * @return A copy of the resulting vector.
 */
YINLINE Vector3 Vector3_mul_scalar(Vector3 vector_0, f32 scalar) {
    return (Vector3){
        vector_0.x * scalar,
        vector_0.y * scalar,
        vector_0.z * scalar};
}

/**
 * @brief Divides vector_0 by vector_1 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector3 Vector3_div(Vector3 vector_0, Vector3 vector_1) {
    return (Vector3){
        vector_0.x / vector_1.x,
        vector_0.y / vector_1.y,
        vector_0.z / vector_1.z};
}

/**
 * @brief Returns the squared length of the provided vector.
 * 
 * @param vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
YINLINE f32 Vector3_length_squared(Vector3 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

/**
 * @brief Returns the length of the provided vector.
 * 
 * @param vector The vector to retrieve the length of.
 * @return The length.
 */
YINLINE f32 Vector3_length(Vector3 vector) {
    return ysqrt(Vector3_length_squared(vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 * 
 * @param vector A pointer to the vector to be normalized.
 */
YINLINE void Vector3_normalize(Vector3* vector) {
    const f32 length = Vector3_length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 * 
 * @param vector The vector to be normalized.
 * @return A normalized copy of the supplied vector 
 */
YINLINE Vector3 Vector3_normalized(Vector3 vector) {
    Vector3_normalize(&vector);
    return vector;
}

/**
 * @brief Returns the dot product between the provided vectors. Typically used
 * to calculate the difference in direction.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The dot product. 
 */
YINLINE f32 Vector3_dot(Vector3 vector_0, Vector3 vector_1) {
    f32 p = 0;
    p += vector_0.x * vector_1.x;
    p += vector_0.y * vector_1.y;
    p += vector_0.z * vector_1.z;
    return p;
}

/**
 * @brief Calculates and returns the cross product of the supplied vectors.
 * The cross product is a new vector which is orthoganal to both provided vectors.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The cross product. 
 */
YINLINE Vector3 Vector3_cross(Vector3 vector_0, Vector3 vector_1) {
    return (Vector3){
        vector_0.y * vector_1.z - vector_0.z * vector_1.y,
        vector_0.z * vector_1.x - vector_0.x * vector_1.z,
        vector_0.x * vector_1.y - vector_0.y * vector_1.x};
}

/**
 * @brief Compares all elements of vector_0 and vector_1 and ensures the difference
 * is less than tolerance.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @param tolerance The difference tolerance. Typically Y_FLOAT_EPSILON or similar.
 * @return True if within tolerance; otherwise false. 
 */
YINLINE const b8 Vector3_compare(Vector3 vector_0, Vector3 vector_1, f32 tolerance) {
    if (yabs(vector_0.x - vector_1.x) > tolerance) {
        return false;
    }

    if (yabs(vector_0.y - vector_1.y) > tolerance) {
        return false;
    }

    if (yabs(vector_0.z - vector_1.z) > tolerance) {
        return false;
    }

    return true;
}

/**
 * @brief Returns the distance between vector_0 and vector_1.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The distance between vector_0 and vector_1.
 */
YINLINE f32 Vector3_distance(Vector3 vector_0, Vector3 vector_1) {
    Vector3 d = (Vector3){
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y,
        vector_0.z - vector_1.z};
    return Vector3_length(d);
}


// ------------------------------------------
// Vector 4
// ------------------------------------------

/**
 * @brief Creates and returns a new 4-element vector using the supplied values.
 * 
 * @param x The x value.
 * @param y The y value.
 * @param z The z value.
 * @param w The w value.
 * @return A new 4-element vector.
 */
YINLINE Vector4 Vector4_create(f32 x, f32 y, f32 z, f32 w) {
    Vector4 out_vector;
#if defined(YUSE_SIMD)
    out_vector.data = _mm_setr_ps(x, y, z, w);
#else
    out_vector.x = x;
    out_vector.y = y;
    out_vector.z = z;
    out_vector.w = w;
#endif
    return out_vector;
}

/**
 * @brief Returns a new Vector3 containing the x, y and z components of the 
 * supplied Vector4, essentially dropping the w component.
 * 
 * @param vector The 4-component vector to extract from.
 * @return A new Vector3 
 */
YINLINE Vector3 Vector4_to_Vector3(Vector4 vector) {
    return (Vector3){vector.x, vector.y, vector.z};
}

/**
 * @brief Returns a new Vector4 using vector as the x, y and z components and w for w.
 * 
 * @param vector The 3-component vector.
 * @param w The w component.
 * @return A new Vector4 
 */
YINLINE Vector4 Vector4_from_Vector3(Vector3 vector, f32 w) {
#if defined(YUSE_SIMD)
    Vector4 out_vector;
    out_vector.data = _mm_setr_ps(x, y, z, w);
    return out_vector;
#else
    return (Vector4){vector.x, vector.y, vector.z, w};
#endif
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 0.0f.
 */
YINLINE Vector4 Vector4_zero() {
    return (Vector4){0.0f, 0.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 1.0f.
 */
YINLINE Vector4 Vector4_one() {
    return (Vector4){1.0f, 1.0f, 1.0f, 1.0f};
}

/**
 * @brief Adds vector_1 to vector_0 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector4 Vector4_add(Vector4 vector_0, Vector4 vector_1) {
    Vector4 result;
     for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector_0.elements[i] + vector_1.elements[i];
    }
    return result;
}

/**
 * @brief Subtracts vector_1 from vector_0 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector4 Vector4_sub(Vector4 vector_0, Vector4 vector_1) {
    Vector4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector_0.elements[i] - vector_1.elements[i];
    }
    return result;
}

/**
 * @brief Multiplies vector_0 by vector_1 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector4 Vector4_mul(Vector4 vector_0, Vector4 vector_1) {
    Vector4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector_0.elements[i] * vector_1.elements[i];
    }
    return result;
}

/**
 * @brief Divides vector_0 by vector_1 and returns a copy of the result.
 * 
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @return The resulting vector. 
 */
YINLINE Vector4 Vector4_div(Vector4 vector_0, Vector4 vector_1) {
    Vector4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector_0.elements[i] / vector_1.elements[i];
    }
    return result;
}

/**
 * @brief Returns the squared length of the provided vector.
 * 
 * @param vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
YINLINE f32 Vector4_length_squared(Vector4 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
}

/**
 * @brief Returns the length of the provided vector.
 * 
 * @param vector The vector to retrieve the length of.
 * @return The length.
 */
YINLINE f32 Vector4_length(Vector4 vector) {
    return ysqrt(Vector4_length_squared(vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 * 
 * @param vector A pointer to the vector to be normalized.
 */
YINLINE void Vector4_normalize(Vector4* vector) {
    const f32 length = Vector4_length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
    vector->w /= length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 * 
 * @param vector The vector to be normalized.
 * @return A normalized copy of the supplied vector 
 */
YINLINE Vector4 Vector4_normalized(Vector4 vector) {
    Vector4_normalize(&vector);
    return vector;
}

YINLINE f32 Vector4_dot_f32(
    f32 a0, f32 a1, f32 a2, f32 a3,
    f32 b0, f32 b1, f32 b2, f32 b3) {
    f32 p;
    p =
        a0 * b0 +
        a1 * b1 +
        a2 * b2 +
        a3 * b3;
    return p;
}


/**
 * @brief Creates and returns an identity matrix:
 * 
 * {
 *   {1, 0, 0, 0},
 *   {0, 1, 0, 0},
 *   {0, 0, 1, 0},
 *   {0, 0, 0, 1}
 * }
 * 
 * @return A new identity matrix 
 */
YINLINE Matrice4 Matrice4_identity() {
    Matrice4 out_matrix;
    yzero_memory(out_matrix.data, sizeof(f32) * 16);
    out_matrix.data[0] = 1.0f;
    out_matrix.data[5] = 1.0f;
    out_matrix.data[10] = 1.0f;
    out_matrix.data[15] = 1.0f;
    return out_matrix;
}

/**
 * @brief Returns the result of multiplying matrix_0 and matrix_1.
 * 
 * @param matrix_0 The first matrix to be multiplied.
 * @param matrix_1 The second matrix to be multiplied.
 * @return The result of the matrix multiplication.
 */
YINLINE Matrice4 Matrice4_mul(Matrice4 matrix_0, Matrice4 matrix_1) {
    Matrice4 out_matrix = Matrice4_identity();

    const f32* m1_ptr = matrix_0.data;
    const f32* m2_ptr = matrix_1.data;
    f32* dst_ptr = out_matrix.data;

    for (i32 i = 0; i < 4; ++i) {
        for (i32 j = 0; j < 4; ++j) {
            *dst_ptr =
                m1_ptr[0] * m2_ptr[0 + j] +
                m1_ptr[1] * m2_ptr[4 + j] +
                m1_ptr[2] * m2_ptr[8 + j] +
                m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;
        }
        m1_ptr += 4;
    }
    return out_matrix;
}

/**
 * @brief Creates and returns an orthographic projection matrix. Typically used to
 * render flat or 2D scenes.
 * 
 * @param left The left side of the view frustum.
 * @param right The right side of the view frustum.
 * @param bottom The bottom side of the view frustum.
 * @param top The top side of the view frustum.
 * @param near_clip The near clipping plane distance.
 * @param far_clip The far clipping plane distance.
 * @return A new orthographic projection matrix. 
 */
YINLINE Matrice4 Matrice4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_clip, f32 far_clip) {
    Matrice4 out_matrix = Matrice4_identity();

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near_clip - far_clip);

    out_matrix.data[0] = -2.0f * lr;
    out_matrix.data[5] = -2.0f * bt;
    out_matrix.data[10] = 2.0f * nf;

    out_matrix.data[12] = (left + right) * lr;
    out_matrix.data[13] = (top + bottom) * bt;
    out_matrix.data[14] = (far_clip + near_clip) * nf;
    return out_matrix;
}

/**
 * @brief Creates and returns a perspective matrix. Typically used to render 3d scenes.
 * 
 * @param fov_radians The field of view in radians.
 * @param aspect_ratio The aspect ratio.
 * @param near_clip The near clipping plane distance.
 * @param far_clip The far clipping plane distance.
 * @return A new perspective matrix. 
 */
YINLINE Matrice4 Matrice4_perspective(f32 fov_radians, f32 aspect_ratio, f32 near_clip, f32 far_clip) {
    f32 half_tan_fov = ytan(fov_radians * 0.5f);
    Matrice4 out_matrix;
    yzero_memory(out_matrix.data, sizeof(f32) * 16);
    out_matrix.data[0] = 1.0f / (aspect_ratio * half_tan_fov);
    out_matrix.data[5] = 1.0f / half_tan_fov;
    out_matrix.data[10] = -((far_clip + near_clip) / (far_clip - near_clip));
    out_matrix.data[11] = -1.0f;
    out_matrix.data[14] = -((2.0f * far_clip * near_clip) / (far_clip - near_clip));
    return out_matrix;
}

/**
 * @brief Creates and returns a look-at matrix, or a matrix looking 
 * at target from the perspective of position.
 * 
 * @param position The position of the matrix.
 * @param target The position to "look at".
 * @param up The up vector.
 * @return A matrix looking at target from the perspective of position. 
 */
YINLINE Matrice4 Matrice4_look_at(Vector3 position, Vector3 target, Vector3 up) {
    Matrice4 out_matrix;
    Vector3 z_axis;
    z_axis.x = target.x - position.x;
    z_axis.y = target.y - position.y;
    z_axis.z = target.z - position.z;

    z_axis = Vector3_normalized(z_axis);
    Vector3 x_axis = Vector3_normalized(Vector3_cross(z_axis, up));
    Vector3 y_axis = Vector3_cross(x_axis, z_axis);

    out_matrix.data[0] = x_axis.x;
    out_matrix.data[1] = y_axis.x;
    out_matrix.data[2] = -z_axis.x;
    out_matrix.data[3] = 0;
    out_matrix.data[4] = x_axis.y;
    out_matrix.data[5] = y_axis.y;
    out_matrix.data[6] = -z_axis.y;
    out_matrix.data[7] = 0;
    out_matrix.data[8] = x_axis.z;
    out_matrix.data[9] = y_axis.z;
    out_matrix.data[10] = -z_axis.z;
    out_matrix.data[11] = 0;
    out_matrix.data[12] = -Vector3_dot(x_axis, position);
    out_matrix.data[13] = -Vector3_dot(y_axis, position);
    out_matrix.data[14] = Vector3_dot(z_axis, position);
    out_matrix.data[15] = 1.0f;

    return out_matrix;
}

/**
 * @brief Returns a transposed copy of the provided matrix (rows->colums)
 * 
 * @param matrix The matrix to be transposed.
 * @return A transposed copy of of the provided matrix.
 */
YINLINE Matrice4 Matrice4_transposed(Matrice4 matrix) {
    Matrice4 out_matrix = Matrice4_identity();
    out_matrix.data[0] = matrix.data[0];
    out_matrix.data[1] = matrix.data[4];
    out_matrix.data[2] = matrix.data[8];
    out_matrix.data[3] = matrix.data[12];
    out_matrix.data[4] = matrix.data[1];
    out_matrix.data[5] = matrix.data[5];
    out_matrix.data[6] = matrix.data[9];
    out_matrix.data[7] = matrix.data[13];
    out_matrix.data[8] = matrix.data[2];
    out_matrix.data[9] = matrix.data[6];
    out_matrix.data[10] = matrix.data[10];
    out_matrix.data[11] = matrix.data[14];
    out_matrix.data[12] = matrix.data[3];
    out_matrix.data[13] = matrix.data[7];
    out_matrix.data[14] = matrix.data[11];
    out_matrix.data[15] = matrix.data[15];
    return out_matrix;
}

/**
 * @brief Creates and returns an inverse of the provided matrix.
 * 
 * @param matrix The matrix to be inverted.
 * @return A inverted copy of the provided matrix. 
 */
YINLINE Matrice4 Matrice4_inverse(Matrice4 matrix) {
    const f32* m = matrix.data;

    f32 t0 = m[10] * m[15];
    f32 t1 = m[14] * m[11];
    f32 t2 = m[6] * m[15];
    f32 t3 = m[14] * m[7];
    f32 t4 = m[6] * m[11];
    f32 t5 = m[10] * m[7];
    f32 t6 = m[2] * m[15];
    f32 t7 = m[14] * m[3];
    f32 t8 = m[2] * m[11];
    f32 t9 = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];
    f32 t12 = m[8] * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4] * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4] * m[9];
    f32 t17 = m[8] * m[5];
    f32 t18 = m[0] * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0] * m[9];
    f32 t21 = m[8] * m[1];
    f32 t22 = m[0] * m[5];
    f32 t23 = m[4] * m[1];

    Matrice4 out_matrix;
    f32* o = out_matrix.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0] = d * o[0];
    o[1] = d * o[1];
    o[2] = d * o[2];
    o[3] = d * o[3];
    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return out_matrix;
}

YINLINE Matrice4 Matrice4_translation(Vector3 position) {
    Matrice4 out_matrix = Matrice4_identity();
    out_matrix.data[12] = position.x;
    out_matrix.data[13] = position.y;
    out_matrix.data[14] = position.z;
    return out_matrix;
}

/**
 * @brief Returns a scale matrix using the provided scale.
 * 
 * @param scale The 3-component scale.
 * @return A scale matrix.
 */
YINLINE Matrice4 Matrice4_scale(Vector3 scale) {
    Matrice4 out_matrix = Matrice4_identity();
    out_matrix.data[0] = scale.x;
    out_matrix.data[5] = scale.y;
    out_matrix.data[10] = scale.z;
    return out_matrix;
}

YINLINE Matrice4 Matrice4_euler_x(f32 angle_radians) {
    Matrice4 out_matrix = Matrice4_identity();
    f32 c = ycos(angle_radians);
    f32 s = ysin(angle_radians);

    out_matrix.data[5] = c;
    out_matrix.data[6] = s;
    out_matrix.data[9] = -s;
    out_matrix.data[10] = c;
    return out_matrix;
}
YINLINE Matrice4 Matrice4_euler_y(f32 angle_radians) {
    Matrice4 out_matrix = Matrice4_identity();
    f32 c = ycos(angle_radians);
    f32 s = ysin(angle_radians);

    out_matrix.data[0] = c;
    out_matrix.data[2] = -s;
    out_matrix.data[8] = s;
    out_matrix.data[10] = c;
    return out_matrix;
}
YINLINE Matrice4 Matrice4_euler_z(f32 angle_radians) {
    Matrice4 out_matrix = Matrice4_identity();

    f32 c = ycos(angle_radians);
    f32 s = ysin(angle_radians);

    out_matrix.data[0] = c;
    out_matrix.data[1] = s;
    out_matrix.data[4] = -s;
    out_matrix.data[5] = c;
    return out_matrix;
}
YINLINE Matrice4 Matrice4_euler_xyz(f32 x_radians, f32 y_radians, f32 z_radians) {
    Matrice4 rx = Matrice4_euler_x(x_radians);
    Matrice4 ry = Matrice4_euler_y(y_radians);
    Matrice4 rz = Matrice4_euler_z(z_radians);
    Matrice4 out_matrix = Matrice4_mul(rx, ry);
    out_matrix = Matrice4_mul(out_matrix, rz);
    return out_matrix;
}

/**
 * @brief Returns a forward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
YINLINE Vector3 Matrice4_forward(Matrice4 matrix) {
    Vector3 forward;
    forward.x = -matrix.data[2];
    forward.y = -matrix.data[6];
    forward.z = -matrix.data[10];
    Vector3_normalize(&forward);
    return forward;
}

/**
 * @brief Returns a backward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
YINLINE Vector3 Matrice4_backward(Matrice4 matrix) {
    Vector3 backward;
    backward.x = matrix.data[2];
    backward.y = matrix.data[6];
    backward.z = matrix.data[10];
    Vector3_normalize(&backward);
    return backward;
}

/**
 * @brief Returns a upward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
YINLINE Vector3 Matrice4_up(Matrice4 matrix) {
    Vector3 up;
    up.x = matrix.data[1];
    up.y = matrix.data[5];
    up.z = matrix.data[9];
    Vector3_normalize(&up);
    return up;
}

/**
 * @brief Returns a downward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
YINLINE Vector3 Matrice4_down(Matrice4 matrix) {
    Vector3 down;
    down.x = -matrix.data[1];
    down.y = -matrix.data[5];
    down.z = -matrix.data[9];
    Vector3_normalize(&down);
    return down;
}

/**
 * @brief Returns a left vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
YINLINE Vector3 Matrice4_left(Matrice4 matrix) {
    Vector3 left;
    left.x = -matrix.data[0];
    left.y = -matrix.data[4];
    left.z = -matrix.data[8];
    Vector3_normalize(&left);
    return left;
}

/**
 * @brief Returns a right vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
YINLINE Vector3 Matrice4_right(Matrice4 matrix) {
    Vector3 right;
    right.x = matrix.data[0];
    right.y = matrix.data[4];
    right.z = matrix.data[8];
    Vector3_normalize(&right);
    return right;
}

// ------------------------------------------
// Quaternion
// ------------------------------------------

YINLINE Quaternion Quaternion_identity() {
    return (Quaternion){0, 0, 0, 1.0f};
}

YINLINE f32 Quaternion_normal(Quaternion q) {
    return ysqrt(
        q.x * q.x +
        q.y * q.y +
        q.z * q.z +
        q.w * q.w);
}

YINLINE Quaternion Quaternion_normalize(Quaternion q) {
    f32 normal = Quaternion_normal(q);
    return (Quaternion){
        q.x / normal,
        q.y / normal,
        q.z / normal,
        q.w / normal};
}

YINLINE Quaternion Quaternion_conjugate(Quaternion q) {
    return (Quaternion){
        -q.x,
        -q.y,
        -q.z,
        q.w};
}

YINLINE Quaternion Quaternion_inverse(Quaternion q) {
    return Quaternion_normalize(Quaternion_conjugate(q));
}

YINLINE Quaternion Quaternion_mul(Quaternion q_0, Quaternion q_1) {
    Quaternion out_Quaternionernion;

    out_Quaternionernion.x = q_0.x * q_1.w +
                       q_0.y * q_1.z -
                       q_0.z * q_1.y +
                       q_0.w * q_1.x;

    out_Quaternionernion.y = -q_0.x * q_1.z +
                       q_0.y * q_1.w +
                       q_0.z * q_1.x +
                       q_0.w * q_1.y;

    out_Quaternionernion.z = q_0.x * q_1.y -
                       q_0.y * q_1.x +
                       q_0.z * q_1.w +
                       q_0.w * q_1.z;

    out_Quaternionernion.w = -q_0.x * q_1.x -
                       q_0.y * q_1.y -
                       q_0.z * q_1.z +
                       q_0.w * q_1.w;

    return out_Quaternionernion;
}

YINLINE f32 Quaternion_dot(Quaternion q_0, Quaternion q_1) {
    return q_0.x * q_1.x +
           q_0.y * q_1.y +
           q_0.z * q_1.z +
           q_0.w * q_1.w;
}

YINLINE Matrice4 Quaternion_to_Matrice4(Quaternion q) {
    Matrice4 out_matrix = Matrice4_identity();

    // https://stackoverflow.com/questions/1556260/convert-Quaternionernion-rotation-to-rotation-matrix

    Quaternion n = Quaternion_normalize(q);

    out_matrix.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    out_matrix.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    out_matrix.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

    out_matrix.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    out_matrix.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    out_matrix.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

    out_matrix.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
    out_matrix.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
    out_matrix.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return out_matrix;
}

// Calculates a rotation matrix based on the Quaternionernion and the passed in center point.
YINLINE Matrice4 Quaternion_to_rotation_matrix(Quaternion q, Vector3 center) {
    Matrice4 out_matrix;

    f32* o = out_matrix.data;
    o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;
    return out_matrix;
}

YINLINE Quaternion Quaternion_from_axis_angle(Vector3 axis, f32 angle, b8 normalize) {
    const f32 half_angle = 0.5f * angle;
    f32 s = ysin(half_angle);
    f32 c = ycos(half_angle);

    Quaternion q = (Quaternion){s * axis.x, s * axis.y, s * axis.z, c};
    if (normalize) {
        return Quaternion_normalize(q);
    }
    return q;
}

YINLINE Quaternion Quaternion_slerp(Quaternion q_0, Quaternion q_1, f32 percentage) {
    Quaternion out_Quaternionernion;
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit Quaternionernions are valid rotations.
    // Normalize to avoid undefined behavior.
    Quaternion v0 = Quaternion_normalize(q_0);
    Quaternion v1 = Quaternion_normalize(q_1);

    // Compute the cosine of the angle between the two vectors.
    f32 dot = Quaternion_dot(v0, v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by
    // reversing one Quaternionernion.
    if (dot < 0.0f) {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // If the inputs are too close for comfort, linearly interpolate
        // and normalize the result.
        out_Quaternionernion = (Quaternion){
            v0.x + ((v1.x - v0.x) * percentage),
            v0.y + ((v1.y - v0.y) * percentage),
            v0.z + ((v1.z - v0.z) * percentage),
            v0.w + ((v1.w - v0.w) * percentage)};

        return Quaternion_normalize(out_Quaternionernion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    f32 theta_0 = yacos(dot);          // theta_0 = angle between input vectors
    f32 theta = theta_0 * percentage;  // theta = angle between v0 and result
    f32 sin_theta = ysin(theta);       // compute this value only once
    f32 sin_theta_0 = ysin(theta_0);   // compute this value only once

    f32 s0 = ycos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
    f32 s1 = sin_theta / sin_theta_0;

    return (Quaternion){
        (v0.x * s0) + (v1.x * s1),
        (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1),
        (v0.w * s0) + (v1.w * s1)};
}

/**
 * @brief Converts provided degrees to radians.
 * 
 * @param degrees The degrees to be converted.
 * @return The amount in radians.
 */
YINLINE f32 deg_to_rad(f32 degrees) {
    return degrees * Y_DEG2RAD_MULTIPLIER;
}

/**
 * @brief Converts provided radians to degrees.
 * 
 * @param radians The radians to be converted.
 * @return The amount in degrees.
 */
YINLINE f32 rad_to_deg(f32 radians) {
    return radians * Y_RAD2DEG_MULTIPLIER;
}