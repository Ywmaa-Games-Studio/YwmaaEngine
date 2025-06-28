#include "camera.h"

#include "math/ymath.h"

Camera camera_create(void) {
    Camera c;
    camera_reset(&c);
    return c;
}

void camera_reset(Camera* c) {
    if (c) {
        c->euler_rotation = Vector3_zero();
        c->position = Vector3_zero();
        c->is_dirty = false;
        c->view_matrix = Matrice4_identity();
    }
}

Vector3 camera_position_get(const Camera* c) {
    if (c) {
        return c->position;
    }
    return Vector3_zero();
}

void camera_position_set(Camera* c, Vector3 position) {
    if (c) {
        c->position = position;
        c->is_dirty = true;
    }
}

Vector3 camera_rotation_euler_get(const Camera* c) {
    if (c) {
        return c->euler_rotation;
    }
    return Vector3_zero();
}

void camera_rotation_euler_set(Camera* c, Vector3 rotation) {
    if (c) {
        c->euler_rotation = rotation;
        c->is_dirty = true;
    }
}

Matrice4 camera_view_get(Camera* c) {
    if (c) {
        if (c->is_dirty) {
            Matrice4 rotation = Matrice4_euler_xyz(c->euler_rotation.x, c->euler_rotation.y, c->euler_rotation.z);
            Matrice4 translation = Matrice4_translation(c->position);

            c->view_matrix = Matrice4_mul(rotation, translation);
            c->view_matrix = Matrice4_inverse(c->view_matrix);

            c->is_dirty = false;
        }
        return c->view_matrix;
    }
    return Matrice4_identity();
}

Vector3 camera_forward(Camera* c) {
    if (c) {
        Matrice4 view = camera_view_get(c);
        return Matrice4_forward(view);
    }
    return Vector3_zero();
}

Vector3 camera_backward(Camera* c) {
    if (c) {
        Matrice4 view = camera_view_get(c);
        return Matrice4_backward(view);
    }
    return Vector3_zero();
}

Vector3 camera_left(Camera* c) {
    if (c) {
        Matrice4 view = camera_view_get(c);
        return Matrice4_left(view);
    }
    return Vector3_zero();
}

Vector3 camera_right(Camera* c) {
    if (c) {
        Matrice4 view = camera_view_get(c);
        return Matrice4_right(view);
    }
    return Vector3_zero();
}

void camera_move_forward(Camera* c, f32 amount) {
    if (c) {
        Vector3 direction = camera_forward(c);
        direction = Vector3_mul_scalar(direction, amount);
        c->position = Vector3_add(c->position, direction);
        c->is_dirty = true;
    }
}

void camera_move_backward(Camera* c, f32 amount) {
    if (c) {
        Vector3 direction = camera_backward(c);
        direction = Vector3_mul_scalar(direction, amount);
        c->position = Vector3_add(c->position, direction);
        c->is_dirty = true;
    }
}

void camera_move_left(Camera* c, f32 amount) {
    if (c) {
        Vector3 direction = camera_left(c);
        direction = Vector3_mul_scalar(direction, amount);
        c->position = Vector3_add(c->position, direction);
        c->is_dirty = true;
    }
}

void camera_move_right(Camera* c, f32 amount) {
    if (c) {
        Vector3 direction = camera_right(c);
        direction = Vector3_mul_scalar(direction, amount);
        c->position = Vector3_add(c->position, direction);
        c->is_dirty = true;
    }
}

void camera_move_up(Camera* c, f32 amount) {
    if (c) {
        Vector3 direction = Vector3_up();
        direction = Vector3_mul_scalar(direction, amount);
        c->position = Vector3_add(c->position, direction);
        c->is_dirty = true;
    }
}

void camera_move_down(Camera* c, f32 amount) {
    if (c) {
        Vector3 direction = Vector3_down();
        direction = Vector3_mul_scalar(direction, amount);
        c->position = Vector3_add(c->position, direction);
        c->is_dirty = true;
    }
}

void camera_yaw(Camera* c, f32 amount) {
    if (c) {
        c->euler_rotation.y += amount;
        c->is_dirty = true;
    }
}

void camera_pitch(Camera* c, f32 amount) {
    if (c) {
        c->euler_rotation.x += amount;

        // Clamp to avoid Gimball lock.
        static const f32 limit = 1.55334306f;  // 89 degrees, or equivalent to deg_to_rad(89.0f);
        c->euler_rotation.x = YCLAMP(c->euler_rotation.x, -limit, limit);

        c->is_dirty = true;
    }
}
