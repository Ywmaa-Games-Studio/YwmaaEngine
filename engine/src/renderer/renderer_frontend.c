#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "math/ymath.h"

#include "resources/resource_types.h"

typedef struct RENDERER_SYSTEM_STATE {
    RENDERER_BACKEND backend;
    Matrice4 projection;
    Matrice4 view;
    f32 near_clip;
    f32 far_clip;

    TEXTURE default_texture;
} RENDERER_SYSTEM_STATE;

static RENDERER_SYSTEM_STATE* state_ptr;

b8 renderer_system_init(u64* memory_requirement, void* state, const char* application_name, E_RENDERER_BACKEND_API rendering_backend_api) {
    *memory_requirement = sizeof(RENDERER_SYSTEM_STATE);
    if (state == 0) {
        return true;
    }
    state_ptr = state;

    renderer_backend_create(rendering_backend_api, &state_ptr->backend);
    state_ptr->backend.frame_number = 0;

    if (!state_ptr->backend.init(&state_ptr->backend, application_name)) {
        PRINT_ERROR("Renderer backend failed to initialize. Shutting down.");
        return false;
    }

    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    state_ptr->projection = Matrice4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, state_ptr->near_clip, state_ptr->far_clip);

    state_ptr->view = Matrice4_translation((Vector3){0, 0, -30.0f});
    state_ptr->view = Matrice4_inverse(state_ptr->view);

    // NOTE: Create default texture, a 256x256 blue/white checkerboard pattern.
    // This is done in code to eliminate asset dependencies.
    PRINT_DEBUG("Creating default texture...");
    const u32 tex_dimension = 256;
    const u32 channels = 4;
    const u32 pixel_count = tex_dimension * tex_dimension;
    u8 pixels[pixel_count * channels];
    //u8* pixels = kallocate(sizeof(u8) * pixel_count * bpp, MEMORY_TAG_TEXTURE);
    yset_memory(pixels, 255, sizeof(u8) * pixel_count * channels);

    // Each pixel.
    for (u64 row = 0; row < tex_dimension; ++row) {
        for (u64 col = 0; col < tex_dimension; ++col) {
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
        }
    }
    renderer_create_texture(
        "default",
        false,
        tex_dimension,
        tex_dimension,
        4,
        pixels,
        false,
        &state_ptr->default_texture);


    return true;
}

void renderer_system_shutdown(void* state) {
    if (state_ptr) {
        renderer_destroy_texture(&state_ptr->default_texture);

        state_ptr->backend.shutdown(&state_ptr->backend);
    }
    state_ptr = 0;
}

b8 renderer_begin_frame(f32 delta_time) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time) {
    if (!state_ptr) {
        return false;
    }
    b8 result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
    state_ptr->backend.frame_number++;
    return result;
}

void renderer_on_resized(u16 width, u16 height) {
    if (state_ptr) {
        state_ptr->backend.resized(&state_ptr->backend, width, height);
    } else {
        PRINT_WARNING("renderer backend does not exist to accept resize: %i %i", width, height);
    }
}

b8 renderer_draw_frame(RENDER_PACKET* packet) {
    // If the begin frame returned successfully, mid-frame operations may continue.
    if (renderer_begin_frame(packet->delta_time)) {
        state_ptr->backend.update_global_state(state_ptr->projection, state_ptr->view, Vector3_zero(), Vector4_one(), 0);

        Matrice4 model = Matrice4_translation((Vector3){0, 0, 0});
        // static f32 angle = 0.01f;
        // angle += 0.001f;
        // quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        // mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());
        GEOMETRY_RENDER_DATA data = {};
        data.object_id = 0;  // TODO: actual object id
        data.model = model;
        data.textures[0] = &state_ptr->default_texture;
        state_ptr->backend.update_object(data);

        // End the frame. If this fails, it is likely unrecoverable.
        b8 result = renderer_end_frame(packet->delta_time);

        if (!result) {
            PRINT_ERROR("renderer_end_frame failed. Application shutting down...");
            return false;
        }
    }

    return true;
}

void renderer_set_view(Matrice4 view) {
    state_ptr->view = view;
}

void renderer_create_texture(const char* name, b8 auto_release, i32 width, i32 height, i32 channel_count, const u8* pixels, b8 has_transparency, struct TEXTURE* out_texture) {
    state_ptr->backend.create_texture(name, auto_release, width, height, channel_count, pixels, has_transparency, out_texture);
}

void renderer_destroy_texture(struct TEXTURE* texture) {
    state_ptr->backend.destroy_texture(texture);
}