#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "math/ymath.h"

#include "resources/resource_types.h"
#include "core/ystring.h"
#include "core/event.h"

#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"

typedef struct RENDERER_SYSTEM_STATE {
    RENDERER_BACKEND backend;
    Matrice4 projection;
    Matrice4 view;
    Vector4 ambient_colour;
    Vector3 view_position;
    Matrice4 ui_projection;
    Matrice4 ui_view;
    f32 near_clip;
    f32 far_clip;
    u32 material_shader_id;
    u32 ui_shader_id;
    u32 render_mode;
} RENDERER_SYSTEM_STATE;

static RENDERER_SYSTEM_STATE* state_ptr;

b8 renderer_on_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    switch (code) {
        case EVENT_CODE_SET_RENDER_MODE: {
            RENDERER_SYSTEM_STATE* state = (RENDERER_SYSTEM_STATE*)listener_inst;
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case RENDERER_VIEW_MODE_DEFAULT:
                    PRINT_DEBUG("Renderer mode set to default.");
                    state->render_mode = RENDERER_VIEW_MODE_DEFAULT;
                    break;
                case RENDERER_VIEW_MODE_LIGHTING:
                    PRINT_DEBUG("Renderer mode set to lighting.");
                    state->render_mode = RENDERER_VIEW_MODE_LIGHTING;
                    break;
                case RENDERER_VIEW_MODE_NORMALS:
                    PRINT_DEBUG("Renderer mode set to normals.");
                    state->render_mode = RENDERER_VIEW_MODE_NORMALS;
                    break;
            }
            return true;
        }
    }

    return false;
}

#define CRITICAL_INIT(op, msg) \
    if (!op) {                 \
        PRINT_ERROR(msg);      \
        return false;          \
    }

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
    state_ptr->render_mode = RENDERER_VIEW_MODE_DEFAULT;

    event_register(EVENT_CODE_SET_RENDER_MODE, state, renderer_on_event);

    // Initialize the backend.
    CRITICAL_INIT(state_ptr->backend.init(&state_ptr->backend, application_name), "Renderer backend failed to initialize. Shutting down.");

    // Shaders
    RESOURCE config_resource;
    SHADER_CONFIG* config = 0;

    // Builtin material shader.
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_MATERIAL, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin material shader.");
    config = (SHADER_CONFIG*)config_resource.data;
    CRITICAL_INIT(shader_system_create(config, rendering_backend_api), "Failed to load builtin material shader.");
    resource_system_unload(&config_resource);
    state_ptr->material_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_MATERIAL);
    
    // Builtin UI shader.
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_UI, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin UI shader.");
    config = (SHADER_CONFIG*)config_resource.data;
    CRITICAL_INIT(shader_system_create(config, rendering_backend_api), "Failed to load builtin UI shader.");
    resource_system_unload(&config_resource);
    state_ptr->ui_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_UI);
    
    // World projection/view
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    state_ptr->projection = Matrice4_identity();
    state_ptr->projection = Matrice4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, state_ptr->near_clip, state_ptr->far_clip);
    // TODO: configurable camera starting position.
    state_ptr->view = Matrice4_translation((Vector3){0, 0, -30.0f});
    state_ptr->view = Matrice4_inverse(state_ptr->view);
    // TODO: Obtain from scene
    state_ptr->ambient_colour = (Vector4){0.25f, 0.25f, 0.25f, 1.0f};

    // UI projection/view
    state_ptr->ui_projection = Matrice4_orthographic(0, 1280.0f, 720.0f, 0.0f, -100.f, 100.0f);
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
        state_ptr->ui_projection = Matrice4_orthographic(0, (f32)height, (f32)width, 0, -100.f, 100.0f); // Y-Axis is flipped intentionally
        state_ptr->backend.resized(&state_ptr->backend, width, height);
    } else {
        PRINT_WARNING("renderer backend does not exist to accept resize: %i %i", width, height);
    }
}

b8 renderer_draw_frame(RENDER_PACKET* packet) {
    state_ptr->backend.frame_number++;

    // If the begin frame returned successfully, mid-frame operations may continue.
    if (state_ptr->backend.begin_frame(&state_ptr->backend, packet->delta_time)) {
        //START world renderpass
        if (!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD)) {
            PRINT_ERROR("backend.begin_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Application shutting down...");
            return false;
        }

        if (!shader_system_use_by_id(state_ptr->material_shader_id)) {
            PRINT_ERROR("Failed to use material shader. Render frame failed.");
            return false;
        }

        // Apply globals
        if (!material_system_apply_global(state_ptr->material_shader_id, &state_ptr->projection, &state_ptr->view, &state_ptr->ambient_colour, &state_ptr->view_position, state_ptr->render_mode)) {
            PRINT_ERROR("Failed to use apply globals for material shader. Render frame failed.");
            return false;
        }

        //START Draw geometries.
        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            MATERIAL* m = 0;
            if (packet->geometries[i].geometry->material) {
                m = packet->geometries[i].geometry->material;
            } else {
                m = material_system_get_default();
            }

            // Apply the material if it hasn't already been this frame. This keeps the
            // same material from being updated multiple times.
            b8 needs_update = m->render_frame_number != state_ptr->backend.frame_number;
            if (!material_system_apply_instance(m, needs_update)) {
                PRINT_WARNING("Failed to apply material '%s'. Skipping draw.", m->name);
                continue;
            } else {
                // Sync the frame number.
                m->render_frame_number = state_ptr->backend.frame_number;
            }

            // Apply the locals
            material_system_apply_local(m, &packet->geometries[i].model);

            // Draw it.
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }
        //END Draw geometries.

        if (!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD)) {
            PRINT_ERROR("backend.end_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Application shutting down...");
            return false;
        }
        if (!shader_system_after_renderpass(state_ptr->material_shader_id)) {
            PRINT_ERROR("shader_system_after_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Render frame failed");
            return false;
        }
        //END world renderpass

        //START UI renderpass
        if (!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI)) {
            PRINT_ERROR("backend.begin_renderpass -> BUILTIN_RENDERPASS_UI failed. Application shutting down...");
            return false;
        }

        // Update UI global state
       if (!shader_system_use_by_id(state_ptr->ui_shader_id)) {
            PRINT_ERROR("Failed to use UI shader. Render frame failed.");
            return false;
        }

        // Apply globals
        if (!material_system_apply_global(state_ptr->ui_shader_id, &state_ptr->ui_projection, &state_ptr->ui_view, 0, 0, 0)) {
            PRINT_ERROR("Failed to use apply globals for UI shader. Render frame failed.");
            return false;
        }

        // Draw ui geometries.
        count = packet->ui_geometry_count;
        for (u32 i = 0; i < count; ++i) {
            MATERIAL* m = 0;
            if (packet->ui_geometries[i].geometry->material) {
                m = packet->ui_geometries[i].geometry->material;
            } else {
                m = material_system_get_default();
            }
            // Apply the material
            b8 needs_update = m->render_frame_number != state_ptr->backend.frame_number;
            if (!material_system_apply_instance(m, needs_update)) {
                PRINT_WARNING("Failed to apply UI material '%s'. Skipping draw.", m->name);
                continue;
            } else {
                // Sync the frame number.
                m->render_frame_number = state_ptr->backend.frame_number;
            }

            // Apply the locals
            material_system_apply_local(m, &packet->ui_geometries[i].model);

            // Draw it.
            state_ptr->backend.draw_geometry(packet->ui_geometries[i]);
        }

        if (!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI)) {
            PRINT_ERROR("backend.end_renderpass -> BUILTIN_RENDERPASS_UI failed. Application shutting down...");
            return false;
        }
        if (!shader_system_after_renderpass(state_ptr->ui_shader_id)) {
            PRINT_ERROR("shader_system_after_renderpass -> BUILTIN_RENDERPASS_UI failed. Render frame failed");
            return false;
        }
        //END UI renderpass


        // End the frame. If this fails, it is likely unrecoverable.
        b8 result = state_ptr->backend.end_frame(&state_ptr->backend, packet->delta_time);

        if (!result) {
            PRINT_ERROR("renderer_end_frame failed. Application shutting down...");
            return false;
        }
    }

    return true;
}

void renderer_set_view(Matrice4 view, Vector3 view_position) {
    state_ptr->view = view;
    state_ptr->view_position = view_position;
}

void renderer_texture_create(const u8* pixels, struct TEXTURE* texture) {
    state_ptr->backend.texture_create(pixels, texture);
}

void renderer_texture_destroy(struct TEXTURE* texture) {
    if (!state_ptr) {
        return;
    }
    state_ptr->backend.texture_destroy(texture);
}

void renderer_texture_create_writeable(TEXTURE* t) {
    state_ptr->backend.texture_create_writeable(t);
}

void renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels) {
    state_ptr->backend.texture_write_data(t, offset, size, pixels);
}

void renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height) {
    state_ptr->backend.texture_resize(t, new_width, new_height);
}

b8 renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices) {
    return state_ptr->backend.create_geometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_destroy_geometry(GEOMETRY* geometry) {
    state_ptr->backend.destroy_geometry(geometry);
}


b8 renderer_renderpass_id(const char* name, u8* out_renderpass_id) {
    // TODO: HACK: Need dynamic renderpasses instead of hardcoding them.
    if (strings_equali("renderpass.builtin.world", name)) {
        *out_renderpass_id = BUILTIN_RENDERPASS_WORLD;
        return true;
    } else if (strings_equali("renderpass.builtin.ui", name)) {
        *out_renderpass_id = BUILTIN_RENDERPASS_UI;
        return true;
    }

    PRINT_ERROR("renderer_renderpass_id: No renderpass named '%s'.", name);
    *out_renderpass_id = INVALID_ID_U8;
    return false;
}

b8 renderer_shader_create(SHADER* s, u8 renderpass_id, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages) {
    return state_ptr->backend.shader_create(s, renderpass_id, stage_count, stage_filenames, stages);
}

void renderer_shader_destroy(SHADER* s) {
    state_ptr->backend.shader_destroy(s);
}

b8 renderer_shader_init(SHADER* s) {
    return state_ptr->backend.shader_init(s);
}

b8 renderer_shader_use(SHADER* s) {
    return state_ptr->backend.shader_use(s);
}

b8 renderer_shader_bind_globals(SHADER* s) {
    return state_ptr->backend.shader_bind_globals(s);
}

b8 renderer_shader_bind_instance(SHADER* s, u32 instance_id) {
    return state_ptr->backend.shader_bind_instance(s, instance_id);
}

b8 renderer_shader_apply_globals(SHADER* s) {
    return state_ptr->backend.shader_apply_globals(s);
}

b8 renderer_shader_apply_instance(SHADER* s, b8 needs_update) {
    return state_ptr->backend.shader_apply_instance(s, needs_update);
}

b8 renderer_shader_acquire_instance_resources(SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id) {
    return state_ptr->backend.shader_acquire_instance_resources(s, maps, out_instance_id);
}

b8 renderer_shader_release_instance_resources(SHADER* s, u32 instance_id) {
    return state_ptr->backend.shader_release_instance_resources(s, instance_id);
}

b8 renderer_set_uniform(SHADER* s, SHADER_UNIFORM* uniform, const void* value) {
    return state_ptr->backend.shader_set_uniform(s, uniform, value);
}

b8 renderer_shader_after_renderpass(SHADER* s) {
    return state_ptr->backend.shader_after_renderpass(s);
}

b8 renderer_texture_map_acquire_resources(struct TEXTURE_MAP* map) {
    return state_ptr->backend.texture_map_acquire_resources(map);
}

void renderer_texture_map_release_resources(struct TEXTURE_MAP* map) {
    state_ptr->backend.texture_map_release_resources(map);
}

