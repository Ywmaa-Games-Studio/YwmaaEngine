#include "math_types.h"

/**
 * @brief Creates and returns a new Transform, using a zero
 * vector for position, identity quaternion for rotation, and
 * a one vector for scale. Also has a null parent. Marked dirty
 * by default.
 */
YAPI Transform transform_create(void);

/**
 * @brief Creates a Transform from the given position.
 * Uses a zero rotation and a one scale.
 *
 * @param position The position to be used.
 * @return A new Transform.
 */
YAPI Transform transform_from_position(Vector3 position);

/**
 * @brief Creates a Transform from the given rotation.
 * Uses a zero position and a one scale.
 *
 * @param rotation The rotation to be used.
 * @return A new Transform.
 */
YAPI Transform transform_from_rotation(Quaternion rotation);

/**
 * @brief Creates a Transform from the given position and rotation.
 * Uses a one scale.
 *
 * @param position The position to be used.
 * @param rotation The rotation to be used.
 * @return A new Transform.
 */
YAPI Transform transform_from_position_rotation(Vector3 position, Quaternion rotation);

/**
 * @brief Creates a Transform from the given position, rotation and scale.
 *
 * @param position The position to be used.
 * @param rotation The rotation to be used.
 * @param scale The scale to be used.
 * @return A new Transform.
 */
YAPI Transform transform_from_position_rotation_scale(Vector3 position, Quaternion rotation, Vector3 scale);

/**
 * @brief Returns a pointer to the provided Transform's parent.
 *
 * @param t A pointer to the Transform whose parent to retrieve.
 * @return A pointer to the parent Transform.
 */
YAPI Transform* transform_parent_get(Transform* t);

/**
 * @brief Sets the parent of the provided Transform.
 *
 * @param t A pointer to the Transform whose parent will be set.
 * @param parent A pointer to the parent Transform.
 */
YAPI void transform_parent_set(Transform* t, Transform* parent);

/**
 * @brief Returns the position of the given Transform.
 *
 * @param t A constant pointer whose position to get.
 * @return A copy of the position.
 */
YAPI Vector3 transform_position_get(const Transform* t);

/**
 * @brief Sets the position of the given Transform.
 *
 * @param t A pointer to the Transform to be updated.
 * @param position The position to be set.
 */
YAPI void transform_position_set(Transform* t, Vector3 position);

/**
 * @brief Applies a translation to the given Transform. Not the
 * same as setting.
 *
 * @param t A pointer to the Transform to be updated.
 * @param translation The translation to be applied.
 */
YAPI void transform_translate(Transform* t, Vector3 translation);

/**
 * @brief Returns the rotation of the given Transform.
 *
 * @param t A constant pointer whose rotation to get.
 * @return A copy of the rotation.
 */
YAPI Quaternion transform_rotation_get(const Transform* t);

/**
 * @brief Sets the rotation of the given Transform.
 *
 * @param t A pointer to the Transform to be updated.
 * @param rotation The rotation to be set.
 */
YAPI void transform_rotation_set(Transform* t, Quaternion rotation);

/**
 * @brief Applies a rotation to the given Transform. Not the
 * same as setting.
 *
 * @param t A pointer to the Transform to be updated.
 * @param rotation The rotation to be applied.
 */
YAPI void transform_rotate(Transform* t, Quaternion rotation);

/**
 * @brief Returns the scale of the given Transform.
 *
 * @param t A constant pointer whose scale to get.
 * @return A copy of the scale.
 */
YAPI Vector3 transform_scale_get(const Transform* t);

/**
 * @brief Sets the scale of the given Transform.
 *
 * @param t A pointer to the Transform to be updated.
 * @param scale The scale to be set.
 */
YAPI void transform_scale_set(Transform* t, Vector3 scale);

/**
 * @brief Applies a scale to the given Transform. Not the
 * same as setting.
 *
 * @param t A pointer to the Transform to be updated.
 * @param scale The scale to be applied.
 */
YAPI void transform_scale(Transform* t, Vector3 scale);

/**
 * @brief Sets the position and rotation of the given Transform.
 *
 * @param t A pointer to the Transform to be updated.
 * @param position The position to be set.
 * @param rotation The rotation to be set.
 */
YAPI void transform_position_rotation_set(Transform* t, Vector3 position, Quaternion rotation);

/**
 * @brief Sets the position, rotation and scale of the given Transform.
 *
 * @param t A pointer to the Transform to be updated.
 * @param position The position to be set.
 * @param rotation The rotation to be set.
 * @param scale The scale to be set.
 */
YAPI void transform_position_rotation_scale_set(Transform* t, Vector3 position, Quaternion rotation, Vector3 scale);

/**
 * @brief Applies translation and rotation to the given Transform.
 *
 * @param t A pointer to the Transform to be updated.
 * @param translation The translation to be applied.
 * @param rotation The rotation to be applied.
 * @return YAPI
 */
YAPI void transform_translate_rotate(Transform* t, Vector3 translation, Quaternion rotation);

/**
 * @brief Retrieves the local transformation matrix from the provided Transform.
 * Automatically recalculates the matrix if it is dirty. Otherwise, the already
 * calculated one is returned.
 *
 * @param t A pointer to the Transform whose matrix to retrieve.
 * @return A copy of the local transformation matrix.
 */
YAPI Matrice4 transform_local_get(Transform* t);

/**
 * @brief Obtains the world matrix of the given Transform
 * by examining its parent (if there is one) and multiplying it
 * against the local matrix.
 *
 * @param t A pointer to the Transform whose world matrix to retrieve.
 * @return A copy of the world matrix.
 */
YAPI Matrice4 transform_world_get(Transform* t);
