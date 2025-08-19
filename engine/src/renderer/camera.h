#pragma once

#include "math/math_types.h"

/**
 * @brief Represents a camera that can be used for
 * a variety of things, especially rendering. Ideally,
 * these are created and managed by the camera system.
 */
typedef struct Camera {
    /**
     * @brief The position of this camera.
     * NOTE: Do not set this directly, use camera_positon_set() instead
     * so the view matrix is recalculated when needed.
     */
    Vector3 position;
    /**
     * @brief The rotation of this camera using Euler angles (pitch, yaw, roll).
     * NOTE: Do not set this directly, use camera_rotation_euler_set() instead
     * so the view matrix is recalculated when needed.
     */
    Vector3 euler_rotation;
    /** @brief Internal flag used to determine when the view matrix needs to be rebuilt. */
    b8 is_dirty;

    /**
     * @brief The view matrix of this camera.
     * NOTE: IMPORTANT: Do not get this directly, use camera_view_get() instead
     * so the view matrix is recalculated when needed.
     */
    Matrice4 view_matrix;
} Camera;

/**
 * @brief Creates a new camera with default zero position
 * and rotation, and view identity matrix. Ideally, the
 * camera system should be used to create this instead
 * of doing so directly.
 *
 * @return A copy of a newly-created camera.
 */
YAPI Camera camera_create(void);

/**
 * @brief Defaults the provided camera to default zero
 * rotation and position, and view matrix to identity.
 *
 * @param c A pointer to the camera to be reset.
 */
YAPI void camera_reset(Camera* c);

/**
 * @brief Gets a copy of the camera's position.
 *
 * @param c A constant pointer to a camera.
 * @return A copy of the camera's position.
 */
YAPI Vector3 camera_position_get(const Camera* c);

/**
 * @brief Sets the provided camera's position.
 *
 * @param c A pointer to a camera.
 * @param position The position to be set.
 */
YAPI void camera_position_set(Camera* c, Vector3 position);

/**
 * @brief Gets a copy of the camera's rotation in Euler angles.
 *
 * @param c A constant pointer to a camera.
 * @return A copy of the camera's rotation in Euler angles.
 */
YAPI Vector3 camera_rotation_euler_get(const Camera* c);

/**
 * @brief Sets the provided camera's rotation in Euler angles.
 *
 * @param c A pointer to a camera.
 * @param position The rotation in Euler angles to be set.
 */
YAPI void camera_rotation_euler_set(Camera* c, Vector3 rotation);

/**
 * @brief Obtains a copy of the camera's view matrix. If camera is
 * dirty, a new one is created, set and returned.
 *
 * @param c A pointer to a camera.
 * @return A copy of the up-to-date view matrix.
 */
YAPI Matrice4 camera_view_get(Camera* c);

/**
 * @brief Returns a copy of the camera's forward vector.
 *
 * @param c A pointer to a camera.
 * @return A copy of the camera's forward vector.
 */
YAPI Vector3 camera_forward(Camera* c);

/**
 * @brief Returns a copy of the camera's backward vector.
 *
 * @param c A pointer to a camera.
 * @return A copy of the camera's backward vector.
 */
YAPI Vector3 camera_backward(Camera* c);

/**
 * @brief Returns a copy of the camera's left vector.
 *
 * @param c A pointer to a camera.
 * @return A copy of the camera's left vector.
 */
YAPI Vector3 camera_left(Camera* c);

/**
 * @brief Returns a copy of the camera's right vector.
 *
 * @param c A pointer to a camera.
 * @return A copy of the camera's right vector.
 */
YAPI Vector3 camera_right(Camera* c);

/**
 * @brief Returns a copy of the camera's up vector.
 *
 * @param c A pointer to a camera.
 * @return A copy of the camera's up vector.
 */
YAPI Vector3 camera_up(Camera* c);

/**
 * @brief Moves the camera forward by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to move.
 */
YAPI void camera_move_forward(Camera* c, f32 amount);

/**
 * @brief Moves the camera backward by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to move.
 */
YAPI void camera_move_backward(Camera* c, f32 amount);

/**
 * @brief Moves the camera left by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to move.
 */
YAPI void camera_move_left(Camera* c, f32 amount);

/**
 * @brief Moves the camera right by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to move.
 */
YAPI void camera_move_right(Camera* c, f32 amount);

/**
 * @brief Moves the camera up (straight along the y-axis) by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to move.
 */
YAPI void camera_move_up(Camera* c, f32 amount);

/**
 * @brief Moves the camera down (straight along the y-axis) by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to move.
 */
YAPI void camera_move_down(Camera* c, f32 amount);

/**
 * @brief Adjusts the camera's yaw by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to adjust by.
 */
YAPI void camera_yaw(Camera* c, f32 amount);

/**
 * @brief Adjusts the camera's pitch by the given amount.
 *
 * @param c A pointer to a camera.
 * @param amount The amount to adjust by.
 */
YAPI void camera_pitch(Camera* c, f32 amount);
