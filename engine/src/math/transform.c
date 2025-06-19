#include "transform.h"

#include "ymath.h"

Transform transform_create(void) {
    Transform t;
    transform_set_position_rotation_scale(&t, Vector3_zero(), Quaternion_identity(), Vector3_one());
    t.local = Matrice4_identity();
    t.parent = 0;
    return t;
}

Transform transform_from_position(Vector3 position) {
    Transform t;
    transform_set_position_rotation_scale(&t, position, Quaternion_identity(), Vector3_one());
    t.local = Matrice4_identity();
    t.parent = 0;
    return t;
}

Transform transform_from_rotation(Quaternion rotation) {
    Transform t;
    transform_set_position_rotation_scale(&t, Vector3_zero(), rotation, Vector3_one());
    t.local = Matrice4_identity();
    t.parent = 0;
    return t;
}

Transform transform_from_position_rotation(Vector3 position, Quaternion rotation) {
    Transform t;
    transform_set_position_rotation_scale(&t, position, rotation, Vector3_one());
    t.local = Matrice4_identity();
    t.parent = 0;
    return t;
}

Transform transform_from_position_rotation_scale(Vector3 position, Quaternion rotation, Vector3 scale) {
    Transform t;
    transform_set_position_rotation_scale(&t, position, rotation, scale);
    t.local = Matrice4_identity();
    t.parent = 0;
    return t;
}

Transform* transform_get_parent(Transform* t) {
    if (!t) {
        return 0;
    }
    return t->parent;
}

void transform_set_parent(Transform* t, Transform* parent) {
    if (t) {
        t->parent = parent;
    }
}

Vector3 transform_get_position(const Transform* t) {
    return t->position;
}

void transform_set_position(Transform* t, Vector3 position) {
    t->position = position;
    t->is_dirty = true;
}

void transform_translate(Transform* t, Vector3 translation) {
    t->position = Vector3_add(t->position, translation);
    t->is_dirty = true;
}

Quaternion transform_get_rotation(const Transform* t) {
    return t->rotation;
}

void transform_set_rotation(Transform* t, Quaternion rotation) {
    t->rotation = rotation;
    t->is_dirty = true;
}

void transform_rotate(Transform* t, Quaternion rotation) {
    t->rotation = Quaternion_mul(t->rotation, rotation);
    t->is_dirty = true;
}

Vector3 transform_get_scale(const Transform* t) {
    return t->scale;
}

void transform_set_scale(Transform* t, Vector3 scale) {
    t->scale = scale;
    t->is_dirty = true;
}

void transform_scale(Transform* t, Vector3 scale) {
    t->scale = Vector3_mul(t->scale, scale);
    t->is_dirty = true;
}

void transform_set_position_rotation(Transform* t, Vector3 position, Quaternion rotation) {
    t->position = position;
    t->rotation = rotation;
    t->is_dirty = true;
}

void transform_set_position_rotation_scale(Transform* t, Vector3 position, Quaternion rotation, Vector3 scale) {
    t->position = position;
    t->rotation = rotation;
    t->scale = scale;
    t->is_dirty = true;
}

void transform_translate_rotate(Transform* t, Vector3 translation, Quaternion rotation) {
    t->position = Vector3_add(t->position, translation);
    t->rotation = Quaternion_mul(t->rotation, rotation);
    t->is_dirty = true;
}

Matrice4 transform_get_local(Transform* t) {
    if (t) {
        if (t->is_dirty) {
            Matrice4 tr = Matrice4_mul(Quaternion_to_Matrice4(t->rotation), Matrice4_translation(t->position));
            tr = Matrice4_mul(Matrice4_scale(t->scale), tr);
            t->local = tr;
            t->is_dirty = false;
        }

        return t->local;
    }
    return Matrice4_identity();
}

Matrice4 transform_get_world(Transform* t) {
    if (t) {
        Matrice4 l = transform_get_local(t);
        if (t->parent) {
            Matrice4 p = transform_get_world(t->parent);
            return Matrice4_mul(l, p);
        }
        return l;
    }
    return Matrice4_identity();
}
