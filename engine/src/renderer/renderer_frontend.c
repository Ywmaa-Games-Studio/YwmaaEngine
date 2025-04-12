#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "math/ymath.h"

#include "resources/resource_types.h"

#include "core/ystring.h"
#include "core/event.h"

#include "systems/texture_system.h"
#include "systems/material_system.h"


typedef struct RENDERER_SYSTEM_STATE {
    RENDERER_BACKEND backend;
    Matrice4 projection;
    Matrice4 view;
    f32 near_clip;
    f32 far_clip;

    // TODO: temporary
    MATERIAL* test_material;
    // TODO: end temporary
} RENDERER_SYSTEM_STATE;

static RENDERER_SYSTEM_STATE* state_ptr;

// TODO: temp
b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT data) {
    const char* names[3] = {
        "cobblestone",
        "paving",
        "paving2"};
    static i8 choice = 2;

    // Save off the old name.
    const char* old_name = names[choice];

    choice++;
    choice %= 3;

    // Acquire the new texture.
    state_ptr->test_material->diffuse_map.texture = texture_system_acquire(names[choice], true);
    if (!state_ptr->test_material->diffuse_map.texture) {
        PRINT_WARNING("event_on_debug_event no texture! using default");
        state_ptr->test_material->diffuse_map.texture = texture_system_get_default_texture();
    }

    // Release the old texture.
    texture_system_release(old_name);

    return true;
}
// TODO: end temp


b8 renderer_system_init(u64* memory_requirement, void* state, const char* application_name, E_RENDERER_BACKEND_API rendering_backend_api) {
    *memory_requirement = sizeof(RENDERER_SYSTEM_STATE);
    if (state == 0) {
        return true;
    }
    state_ptr = state;

    // TODO: temp
    event_register(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
    // TODO: end temp


    if (!renderer_backend_create(rendering_backend_api, &state_ptr->backend)){
        PRINT_ERROR("failed to create backend. Shutting down.");
        return false;
    }

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

    return true;
}

void renderer_system_shutdown(void* state) {
    if (state_ptr) {
        // TODO: temp
        event_unregister(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
        // TODO: end temp

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
        data.model = model;
        // TODO: temporary.
        // Create a default material if does not exist.
        if (!state_ptr->test_material) {
            // Automatic config
            state_ptr->test_material = material_system_acquire("test_material");
            if (!state_ptr->test_material) {
                PRINT_WARNING("Automatic material load failed, falling back to manual default material.");
                // Manual config
                MATERIAL_CONFIG config;
                string_ncopy(config.name, "test_material", MATERIAL_NAME_MAX_LENGTH);
                config.auto_release = false;
                config.diffuse_colour = Vector4_one();  // white
                string_ncopy(config.diffuse_map_name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
                state_ptr->test_material = material_system_acquire_from_config(config);
            }
        }

        data.material = state_ptr->test_material;
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

void renderer_create_texture(const u8* pixels, struct TEXTURE* texture) {
    state_ptr->backend.create_texture(pixels, texture);
}

void renderer_destroy_texture(struct TEXTURE* texture) {
    if (!state_ptr) {
        return;
    }
    state_ptr->backend.destroy_texture(texture);
}

b8 renderer_create_material(struct MATERIAL* material) {
    return state_ptr->backend.create_material(material);
}

void renderer_destroy_material(struct MATERIAL* material) {
    state_ptr->backend.destroy_material(material);
}