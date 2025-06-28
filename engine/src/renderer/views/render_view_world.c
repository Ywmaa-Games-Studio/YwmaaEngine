#include "render_view_world.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/event.h"
#include "math/ymath.h"
#include "math/transform.h"
#include "data_structures/darray.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "renderer/renderer_frontend.h"

typedef struct RENDER_VIEW_WORLD_INTERNAL_DATA {
    u32 shader_id;
    f32 fov;
    f32 near_clip;
    f32 far_clip;
    Matrice4 projection_matrix;
    Camera* world_camera;
    Vector4 ambient_color;
    u32 render_mode;
} RENDER_VIEW_WORLD_INTERNAL_DATA;

static b8 render_view_on_event(u16 code, void* sender, void* listener_inst, EVENT_CONTEXT context) {
    RENDER_VIEW* self = (RENDER_VIEW*)listener_inst;
    if (!self) {
        return false;
    }
    RENDER_VIEW_WORLD_INTERNAL_DATA* data = (RENDER_VIEW_WORLD_INTERNAL_DATA*)self->internal_data;
    if (!data) {
        return false;
    }

    switch (code) {
        case EVENT_CODE_SET_RENDER_MODE: {
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case RENDERER_VIEW_MODE_DEFAULT:
                    PRINT_DEBUG("Renderer mode set to default.");
                    data->render_mode = RENDERER_VIEW_MODE_DEFAULT;
                    break;
                case RENDERER_VIEW_MODE_LIGHTING:
                    PRINT_DEBUG("Renderer mode set to lighting.");
                    data->render_mode = RENDERER_VIEW_MODE_LIGHTING;
                    break;
                case RENDERER_VIEW_MODE_NORMALS:
                    PRINT_DEBUG("Renderer mode set to normals.");
                    data->render_mode = RENDERER_VIEW_MODE_NORMALS;
                    break;
            }
            return true;
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}

b8 render_view_world_on_create(struct RENDER_VIEW* self) {
    if (self) {
        self->internal_data = yallocate_aligned(sizeof(RENDER_VIEW_WORLD_INTERNAL_DATA), 8, MEMORY_TAG_RENDERER);
        RENDER_VIEW_WORLD_INTERNAL_DATA* data = self->internal_data;
        // Get either the custom shader override or the defined default.
        data->shader_id = shader_system_get_id(self->custom_shader_name ? self->custom_shader_name : "shader.builtin.material");
        // TODO: Set from configuration.
        data->near_clip = 0.1f;
        data->far_clip = 1000.0f;
        data->fov = deg_to_rad(45.0f);

        // Default
        data->projection_matrix = Matrice4_perspective(data->fov, 1280 / 720.0f, data->near_clip, data->far_clip);

        data->world_camera = camera_system_get_default();

        // TODO: Obtain from scene
        data->ambient_color = (Vector4){0.25f, 0.25f, 0.25f, 1.0f};

        // Listen for mode changes.
        if (!event_register(EVENT_CODE_SET_RENDER_MODE, self, render_view_on_event)) {
            PRINT_ERROR("Unable to listen for render mode set event, creation failed.");
            return false;
        }
        return true;
    }

    PRINT_ERROR("render_view_world_on_create - Requires a valid pointer to a view.");
    return false;
}

void render_view_world_on_destroy(struct RENDER_VIEW* self) {
    if (self && self->internal_data) {
        event_unregister(EVENT_CODE_SET_RENDER_MODE, self, render_view_on_event);
        yfree(self->internal_data, MEMORY_TAG_RENDERER);
        self->internal_data = 0;
    }
}

void render_view_world_on_resize(struct RENDER_VIEW* self, u32 width, u32 height) {
    // Check if different. If so, regenerate projection matrix.
    if (width != self->width || height != self->height) {
        RENDER_VIEW_WORLD_INTERNAL_DATA* data = self->internal_data;

        self->width = width;
        self->height = height;
        f32 aspect = (f32)self->width / self->height;
        data->projection_matrix = Matrice4_perspective(data->fov, aspect, data->near_clip, data->far_clip);

        for (u32 i = 0; i < self->renderpass_count; ++i) {
            self->passes[i]->render_area.x = 0;
            self->passes[i]->render_area.y = 0;
            self->passes[i]->render_area.z = width;
            self->passes[i]->render_area.w = height;
        }
    }
}

b8 render_view_world_on_build_packet(const struct RENDER_VIEW* self, void* data, struct RENDER_VIEW_PACKET* out_packet) {
    if (!self || !data || !out_packet) {
        PRINT_WARNING("render_view_world_on_build_packet requires valid pointer to view, packet, and data.");
        return false;
    }

    MESH_PACKET_DATA* mesh_data = (MESH_PACKET_DATA*)data;
    RENDER_VIEW_WORLD_INTERNAL_DATA* internal_data = (RENDER_VIEW_WORLD_INTERNAL_DATA*)self->internal_data;

    out_packet->geometries = darray_create(GEOMETRY_RENDER_DATA);
    out_packet->view = self;

    // Set matrices, etc.
    out_packet->projection_matrix = internal_data->projection_matrix;
    out_packet->view_matrix = camera_view_get(internal_data->world_camera);
    out_packet->view_position = camera_position_get(internal_data->world_camera);
    out_packet->ambient_color = internal_data->ambient_color;

    // Obtain all geometries from the current scene.
    // Iterate all meshes and add them to the packet's geometries collection
    for (u32 i = 0; i < mesh_data->mesh_count; ++i) {
        Mesh* m = &mesh_data->meshes[i];
        for (u32 j = 0; j < m->geometry_count; ++j) {
            // Only add meshes with _no_ transparency.
            // TODO: Add something to material to check for transparency.
            if ((m->geometries[j]->material->diffuse_map.texture->flags & TEXTURE_FLAG_HAS_TRANSPARENCY) == 0) {
                GEOMETRY_RENDER_DATA render_data;
                render_data.geometry = m->geometries[j];
                render_data.model = transform_get_world(&m->transform);
                darray_push(out_packet->geometries, render_data);
                out_packet->geometry_count++;
            }
        }
    }

    return true;
}

b8 render_view_world_on_render(const struct RENDER_VIEW* self, const struct RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index) {
    RENDER_VIEW_WORLD_INTERNAL_DATA* data = self->internal_data;
    u32 shader_id = data->shader_id;

    for (u32 p = 0; p < self->renderpass_count; ++p) {
        RENDERPASS* pass = self->passes[p];
        if (!renderer_renderpass_begin(pass, &pass->targets[render_target_index])) {
            PRINT_ERROR("render_view_world_on_render pass index %u failed to start.", p);
            return false;
        }

        if (!shader_system_use_by_id(shader_id)) {
            PRINT_ERROR("Failed to use material shader. Render frame failed.");
            return false;
        }

        // Apply globals
        // TODO: Find a generic way to request data such as ambient colour (which should be from a scene),
        // and mode (from the renderer)
        if (!material_system_apply_global(shader_id, &packet->projection_matrix, &packet->view_matrix, &packet->ambient_color, &packet->view_position, data->render_mode)) {
            PRINT_ERROR("Failed to use apply globals for material shader. Render frame failed.");
            return false;
        }

        // Draw geometries.
        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            MATERIAL* m = 0;
            if (packet->geometries[i].geometry->material) {
                m = packet->geometries[i].geometry->material;
            } else {
                m = material_system_get_default();
            }

            // Update the material if it hasn't already been this frame. This keeps the
            // same material from being updated multiple times. It still needs to be bound
            // either way, so this check result gets passed to the backend which either
            // updates the internal shader bindings and binds them, or only binds them.
            b8 needs_update = m->render_frame_number != frame_number;
            if (!material_system_apply_instance(m, needs_update)) {
                PRINT_WARNING("Failed to apply material '%s'. Skipping draw.", m->name);
                continue;
            } else {
                // Sync the frame number.
                m->render_frame_number = frame_number;
            }

            // Apply the locals
            material_system_apply_local(m, &packet->geometries[i].model);

            // Draw it.
            renderer_draw_geometry(&packet->geometries[i]);
        }

        if (!renderer_renderpass_end(pass)) {
            PRINT_ERROR("render_view_world_on_render pass index %u failed to end.", p);
            return false;
        }

        if (!shader_system_after_renderpass(shader_id)) {
            PRINT_ERROR("shader_system_after_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Render frame failed");
            return false;
        }
    }

    return true;
}
