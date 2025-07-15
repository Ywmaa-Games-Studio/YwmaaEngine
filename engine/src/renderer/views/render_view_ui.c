#include "render_view_ui.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/event.h"
#include "math/ymath.h"
#include "math/transform.h"
#include "data_structures/darray.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "renderer/renderer_frontend.h"
#include "resources/ui_text.h"

typedef struct RENDER_VIEW_UI_INTERNAL_DATA {
    u32 shader_id;
    SHADER* shader;
    f32 near_clip;
    f32 far_clip;
    Matrice4 projection_matrix;
    Matrice4 view_matrix;
    u16 diffuse_map_location;
    u16 diffuse_color_location;
    u16 model_location;
    // u32 render_mode;
} RENDER_VIEW_UI_INTERNAL_DATA;

b8 render_view_ui_on_create(struct RENDER_VIEW* self) {
    if (self) {
        self->internal_data = yallocate_aligned(sizeof(RENDER_VIEW_UI_INTERNAL_DATA), 8, MEMORY_TAG_RENDERER);
        RENDER_VIEW_UI_INTERNAL_DATA* data = self->internal_data;

        // Get either the custom shader override or the defined default.
        data->shader_id = shader_system_get_id(self->custom_shader_name ? self->custom_shader_name : "shader.builtin.ui");
        data->shader = shader_system_get_by_id(data->shader_id);
        data->diffuse_map_location = shader_system_uniform_index(data->shader, "diffuse_texture");
        data->diffuse_color_location = shader_system_uniform_index(data->shader, "diffuse_color");
        data->model_location = shader_system_uniform_index(data->shader, "model");
        // TODO: Set from configuration.
        data->near_clip = -100.0f;
        data->far_clip = 100.0f;
        
        // Default
        data->projection_matrix = Matrice4_orthographic(0.0f, 1280.0f, 720.0f, 0.0f, data->near_clip, data->far_clip);
        data->view_matrix = Matrice4_identity();

        return true;
    }
    PRINT_ERROR("render_view_ui_on_create - Requires a valid pointer to a view.");
    return false;
}

void render_view_ui_on_destroy(struct RENDER_VIEW* self) {
    if (self && self->internal_data) {
        yfree(self->internal_data);
        self->internal_data = 0;
    }
}

void render_view_ui_on_resize(struct RENDER_VIEW* self, u32 width, u32 height) {
    // Check if different. If so, regenerate projection matrix.
    if (width != self->width || height != self->height) {
        RENDER_VIEW_UI_INTERNAL_DATA* data = self->internal_data;

        self->width = width;
        self->height = height;
        data->projection_matrix = Matrice4_orthographic(0.0f, (f32)self->width, (f32)self->height, 0.0f, data->near_clip, data->far_clip);

        for (u32 i = 0; i < self->renderpass_count; ++i) {
            self->passes[i]->render_area.x = 0;
            self->passes[i]->render_area.y = 0;
            self->passes[i]->render_area.z = width;
            self->passes[i]->render_area.w = height;
        }
    }
}

b8 render_view_ui_on_build_packet(const struct RENDER_VIEW* self, void* data, struct RENDER_VIEW_PACKET* out_packet) {
    if (!self || !data || !out_packet) {
        PRINT_WARNING("render_view_ui_on_build_packet requires valid pointer to view, packet, and data.");
        return false;
    }

    UI_PACKET_DATA* packet_data = (UI_PACKET_DATA*)data;
    RENDER_VIEW_UI_INTERNAL_DATA* internal_data = (RENDER_VIEW_UI_INTERNAL_DATA*)self->internal_data;

    out_packet->geometries = darray_create(GEOMETRY_RENDER_DATA);
    out_packet->view = self;

    // Set matrices, etc.
    out_packet->projection_matrix = internal_data->projection_matrix;
    out_packet->view_matrix = internal_data->view_matrix;

    // TODO: temp set extended data to the test text objects for now.
    out_packet->extended_data = data;

    // Obtain all geometries from the current scene.
    // Iterate all meshes and add them to the packet's geometries collection
    for (u32 i = 0; i < packet_data->mesh_data.mesh_count; ++i) {
        Mesh* m = packet_data->mesh_data.meshes[i];
        for (u32 j = 0; j < m->geometry_count; ++j) {
            GEOMETRY_RENDER_DATA render_data;
            render_data.geometry = m->geometries[j];
            render_data.model = transform_get_world(&m->transform);
            darray_push(out_packet->geometries, render_data);
            out_packet->geometry_count++;
        }
    }

    return true;
}
void render_view_ui_on_destroy_packet(const struct RENDER_VIEW* self, struct RENDER_VIEW_PACKET* packet) {
    darray_destroy(packet->geometries);
    yzero_memory(packet, sizeof(RENDER_VIEW_PACKET));
}

b8 render_view_ui_on_render(const struct RENDER_VIEW* self, const struct RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index) {
    RENDER_VIEW_UI_INTERNAL_DATA* data = self->internal_data;
    u32 shader_id = data->shader_id;

    for (u32 p = 0; p < self->renderpass_count; ++p) {
        RENDERPASS* pass = self->passes[p];
        if (!renderer_renderpass_begin(pass, &pass->targets[render_target_index])) {
            PRINT_ERROR("render_view_ui_on_render pass index %u failed to start.", p);
            return false;
        }

        if (!shader_system_use_by_id(shader_id)) {
            PRINT_ERROR("Failed to use material shader. Render frame failed.");
            return false;
        }

        // Apply globals
        if (!material_system_apply_global(shader_id, frame_number, &packet->projection_matrix, &packet->view_matrix, 0, 0, 0)) {
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

 // Draw bitmap text
        UI_PACKET_DATA* packet_data = (UI_PACKET_DATA*)packet->extended_data;  // array of texts
        for (u32 i = 0; i < packet_data->text_count; ++i) {
            UI_TEXT* text = packet_data->texts[i];
            shader_system_bind_instance(text->instance_id);

            if (!shader_system_uniform_set_by_index(data->diffuse_map_location, &text->data->atlas)) {
                PRINT_ERROR("Failed to apply bitmap font diffuse map uniform.");
                return false;
            }

            // TODO: font color.
            static Vector4 white_color = (Vector4){1.0f, 1.0f, 1.0f, 1.0f};  // white
            if (!shader_system_uniform_set_by_index(data->diffuse_color_location, &white_color)) {
                PRINT_ERROR("Failed to apply bitmap font diffuse color uniform.");
                return false;
            }
            b8 needs_update = text->render_frame_number != frame_number;
            shader_system_apply_instance(needs_update);

            // Sync the frame number.
            text->render_frame_number = frame_number;

            // Apply the locals
            Matrice4 model = transform_get_world(&text->transform);
            if(!shader_system_uniform_set_by_index(data->model_location, &model)) {
                PRINT_ERROR("Failed to apply model matrix for text");
            }

            ui_text_draw(text);
        }

        if (!renderer_renderpass_end(pass)) {
            PRINT_ERROR("render_view_ui_on_render pass index %u failed to end.", p);
            return false;
        }

        if (!shader_system_after_renderpass(shader_id)) {
            PRINT_ERROR("shader_system_after_renderpass -> BUILTIN_RENDERPASS_UI failed. Render frame failed");
            return false;
        }
    }

    return true;
}
