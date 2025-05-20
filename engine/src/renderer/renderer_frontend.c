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
    Matrice4 ui_projection;
    Matrice4 ui_view;
    f32 near_clip;
    f32 far_clip;
} RENDERER_SYSTEM_STATE;

static RENDERER_SYSTEM_STATE* state_ptr;


b8 renderer_system_init(u64* memory_requirement, void* state, const char* application_name, E_RENDERER_BACKEND_API rendering_backend_api) {
    *memory_requirement = sizeof(RENDERER_SYSTEM_STATE);
    if (state == 0) {
        return true;
    }
    state_ptr = state;

    if (!renderer_backend_create(rendering_backend_api, &state_ptr->backend)){
        PRINT_ERROR("failed to create backend. Shutting down.");
        return false;
    }

    state_ptr->backend.frame_number = 0;

    if (!state_ptr->backend.init(&state_ptr->backend, application_name)) {
        PRINT_ERROR("Renderer backend failed to initialize. Shutting down.");
        return false;
    }
    // World projection/view
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    state_ptr->projection = Matrice4_identity();
    state_ptr->projection = Matrice4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, state_ptr->near_clip, state_ptr->far_clip);
    // TODO: configurable camera starting position.
    state_ptr->view = Matrice4_translation((Vector3){0, 0, -30.0f});
    state_ptr->view = Matrice4_inverse(state_ptr->view);

    // UI projection/view
    state_ptr->ui_projection = Matrice4_orthographic(0, 1280.0f, 720.0f, 0, -100.f, 100.0f);  // Intentionally flipped on y axis.
    state_ptr->ui_view = Matrice4_inverse(Matrice4_identity());

    return true;
}

void renderer_system_shutdown(void* state) {
    if (state_ptr) {

        state_ptr->backend.shutdown(&state_ptr->backend);
    }
    state_ptr = 0;
}

void renderer_on_resized(u16 width, u16 height) {
    if (state_ptr) {
        state_ptr->projection = Matrice4_perspective(deg_to_rad(45.0f), width / (f32)height, state_ptr->near_clip, state_ptr->far_clip);
        state_ptr->ui_projection = Matrice4_orthographic(0, (f32)width, (f32)height, 0, -100.f, 100.0f); // Intentionally flipped on y axis.
        state_ptr->backend.resized(&state_ptr->backend, width, height);
    } else {
        PRINT_WARNING("renderer backend does not exist to accept resize: %i %i", width, height);
    }
}

b8 renderer_draw_frame(RENDER_PACKET* packet) {
    // If the begin frame returned successfully, mid-frame operations may continue.
    if (state_ptr->backend.begin_frame(&state_ptr->backend, packet->delta_time)) {
        //START world renderpass
        if (!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD)) {
            PRINT_ERROR("backend.begin_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Application shutting down...");
            return false;
        }

        state_ptr->backend.update_global_world_state(state_ptr->projection, state_ptr->view, Vector3_zero(), Vector4_one(), 0);

        //START Draw geometries.
        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }
        //END Draw geometries.

        if (!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD)) {
            PRINT_ERROR("backend.end_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Application shutting down...");
            return false;
        }
        //END world renderpass

        //START UI renderpass
        if (!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI)) {
            PRINT_ERROR("backend.begin_renderpass -> BUILTIN_RENDERPASS_UI failed. Application shutting down...");
            return false;
        }

        // Update UI global state
        state_ptr->backend.update_global_ui_state(state_ptr->ui_projection, state_ptr->ui_view, 0);

        // Draw ui geometries.
        count = packet->ui_geometry_count;
        for (u32 i = 0; i < count; ++i) {
            state_ptr->backend.draw_geometry(packet->ui_geometries[i]);
        }

        if (!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI)) {
            PRINT_ERROR("backend.end_renderpass -> BUILTIN_RENDERPASS_UI failed. Application shutting down...");
            return false;
        }
        //END UI renderpass


        // End the frame. If this fails, it is likely unrecoverable.
        b8 result = state_ptr->backend.end_frame(&state_ptr->backend, packet->delta_time);
        state_ptr->backend.frame_number++;

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

b8 renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices) {
    return state_ptr->backend.create_geometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_destroy_geometry(GEOMETRY* geometry) {
    state_ptr->backend.destroy_geometry(geometry);
}
