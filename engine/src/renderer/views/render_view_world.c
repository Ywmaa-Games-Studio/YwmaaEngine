#include "render_view_world.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/event.h"
#include "math/ymath.h"
#include "math/transform.h"
#include "memory/linear_allocator.h"
#include "data_structures/darray.h"
#include "systems/resource_system.h"
#include "systems/material_system.h"
#include "systems/render_view_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "renderer/renderer_frontend.h"

typedef struct RENDER_VIEW_WORLD_INTERNAL_DATA {
    SHADER* shader;
    f32 fov;
    f32 near_clip;
    f32 far_clip;
    Matrice4 projection_matrix;
    Camera* world_camera;
    Vector4 ambient_color;
    u32 render_mode;
} RENDER_VIEW_WORLD_INTERNAL_DATA;

/** @brief A private structure used to sort geometry by distance from the camera. */
typedef struct GEOMETRY_DISTANCE {
    /** @brief The geometry render data. */
    GEOMETRY_RENDER_DATA g;
    /** @brief The distance from the camera. */
    f32 distance;
} GEOMETRY_DISTANCE;

/**
 * @brief A private, recursive, in-place sort function for geometry_distance structures.
 *
 * @param arr The array of geometry_distance structures to be sorted.
 * @param low_index The low index to start the sort from (typically 0)
 * @param high_index The high index to end with (typically the array length - 1)
 * @param ascending True to sort in ascending order; otherwise descending.
 */
static void quick_sort(GEOMETRY_DISTANCE arr[], i32 low_index, i32 high_index, b8 ascending);

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
        case EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED:
            render_view_system_regenerate_render_targets(self);
            // This needs to be consumed by other views, so consider it _not_ handled.
            return false;
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}

b8 render_view_world_on_create(struct RENDER_VIEW* self) {
    if (self) {
        self->internal_data = yallocate_aligned(sizeof(RENDER_VIEW_WORLD_INTERNAL_DATA), 8, MEMORY_TAG_RENDERER);
        RENDER_VIEW_WORLD_INTERNAL_DATA* data = self->internal_data;

        // TODO: move to material system and get a reference here instead.
        // Builtin material shader.
        const char* shader_name = "shader.builtin.material";
        RESOURCE config_resource;
        if (!resource_system_load(shader_name, RESOURCE_TYPE_SHADER, 0, &config_resource)) {
            PRINT_ERROR("Failed to load builtin material shader.");
            return false;
        }
        SHADER_CONFIG* config = (SHADER_CONFIG*)config_resource.data;
        // NOTE: Assuming the first pass since that's all this view has.
        if (!shader_system_create(&self->passes[0], config, renderer_get_backend_api())) {
            PRINT_ERROR("Failed to load builtin material shader.");
            return false;
        }
        resource_system_unload(&config_resource);
        // Get either the custom shader override or the defined default.
        data->shader = shader_system_get(self->custom_shader_name ? self->custom_shader_name : shader_name);
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

        if (!event_register(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, self, render_view_on_event)) {
            PRINT_ERROR("Unable to listen for refresh required event, creation failed.");
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

        // Unregister from the event.
        event_unregister(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, self, render_view_on_event);

        yfree(self->internal_data);
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
            self->passes[i].render_area.x = 0;
            self->passes[i].render_area.y = 0;
            self->passes[i].render_area.z = width;
            self->passes[i].render_area.w = height;
        }
    }
}

b8 render_view_world_on_build_packet(const struct RENDER_VIEW* self, struct LINEAR_ALLOCATOR* frame_allocator, void* data, struct RENDER_VIEW_PACKET* out_packet) {
    if (!self || !data || !out_packet) {
        PRINT_WARNING("render_view_world_on_build_packet requires valid pointer to view, packet, and data.");
        return false;
    }

    GEOMETRY_RENDER_DATA* geometry_data = (GEOMETRY_RENDER_DATA*)data;
    RENDER_VIEW_WORLD_INTERNAL_DATA* internal_data = (RENDER_VIEW_WORLD_INTERNAL_DATA*)self->internal_data;

    out_packet->geometries = darray_create(GEOMETRY_RENDER_DATA);
    out_packet->view = self;

    // Set matrices, etc.
    out_packet->projection_matrix = internal_data->projection_matrix;
    out_packet->view_matrix = camera_view_get(internal_data->world_camera);
    out_packet->view_position = camera_position_get(internal_data->world_camera);
    out_packet->ambient_color = internal_data->ambient_color;

    // Obtain all geometries from the current scene.

    GEOMETRY_DISTANCE* geometry_distances = darray_create(GEOMETRY_DISTANCE);

    u32 geometry_data_count = darray_length(geometry_data);
    for (u32 i = 0; i < geometry_data_count; ++i) {
        GEOMETRY_RENDER_DATA* g_data = &geometry_data[i];
        if(!g_data->geometry) {
            continue;
        }
        
        // TODO: Add something to material to check for transparency.
        if ((g_data->geometry->material->diffuse_map.texture->flags & TEXTURE_FLAG_HAS_TRANSPARENCY) == 0) {
            // Only add meshes with _no_ transparency.
            darray_push(out_packet->geometries, geometry_data[i]);
            out_packet->geometry_count++;
        } else {
            // For meshes _with_ transparency, add them to a separate list to be sorted by distance later.
            // Get the center, extract the global position from the model matrix and add it to the center,
            // then calculate the distance between it and the camera, and finally save it to a list to be sorted.
            // NOTE: This isn't perfect for translucent meshes that intersect, but is enough for our purposes now.
            Vector3 center = Vector3_transform(g_data->geometry->center, g_data->model);
            f32 distance = Vector3_distance(center, internal_data->world_camera->position);

            GEOMETRY_DISTANCE gdist;
            gdist.distance = yabs(distance);
            gdist.g = geometry_data[i];

            darray_push(geometry_distances, gdist);
        }
    }

    // Sort the distances
    u32 geometry_count = darray_length(geometry_distances);
    quick_sort(geometry_distances, 0, geometry_count - 1, false);

    // Add them to the packet geometry.
    for (u32 i = 0; i < geometry_count; ++i) {
        darray_push(out_packet->geometries, geometry_distances[i].g);
        out_packet->geometry_count++;
    }

    // Clean up.
    darray_destroy(geometry_distances);

    return true;
}

void render_view_world_on_destroy_packet(const struct RENDER_VIEW* self, struct RENDER_VIEW_PACKET* packet) {
    darray_destroy(packet->geometries);
    yzero_memory(packet, sizeof(RENDER_VIEW_PACKET));
}

b8 render_view_world_on_render(const struct RENDER_VIEW* self, const struct RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index) {
    RENDER_VIEW_WORLD_INTERNAL_DATA* data = self->internal_data;
    u32 shader_id = data->shader->id;

    for (u32 p = 0; p < self->renderpass_count; ++p) {
        RENDERPASS* pass = &self->passes[p];
        if (!renderer_renderpass_begin(pass, &pass->targets[render_target_index])) {
            PRINT_ERROR("render_view_world_on_render pass index %u failed to start.", p);
            return false;
        }

        if (!shader_system_use_by_id(shader_id)) {
            PRINT_ERROR("Failed to use material shader. Render frame failed.");
            return false;
        }

        // Apply globals
        // TODO: Find a generic way to request data such as ambient color (which should be from a scene),
        // and mode (from the renderer)
        if (!material_system_apply_global(shader_id, frame_number, &packet->projection_matrix, &packet->view_matrix, &packet->ambient_color, &packet->view_position, data->render_mode)) {
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

// Quicksort for geometry_distance

static void swap(GEOMETRY_DISTANCE* a, GEOMETRY_DISTANCE* b) {
    GEOMETRY_DISTANCE temp = *a;
    *a = *b;
    *b = temp;
}

static i32 partition(GEOMETRY_DISTANCE arr[], i32 low_index, i32 high_index, b8 ascending) {
    GEOMETRY_DISTANCE pivot = arr[high_index];
    i32 i = (low_index - 1);

    for (i32 j = low_index; j <= high_index - 1; ++j) {
        if (ascending) {
            if (arr[j].distance < pivot.distance) {
                ++i;
                swap(&arr[i], &arr[j]);
            }
        } else {
            if (arr[j].distance > pivot.distance) {
                ++i;
                swap(&arr[i], &arr[j]);
            }
        }
    }
    swap(&arr[i + 1], &arr[high_index]);
    return i + 1;
}

static void quick_sort(GEOMETRY_DISTANCE arr[], i32 low_index, i32 high_index, b8 ascending) {
    if (low_index < high_index) {
        i32 partition_index = partition(arr, low_index, high_index, ascending);

        // Independently sort elements before and after the partition index.
        quick_sort(arr, low_index, partition_index - 1, ascending);
        quick_sort(arr, partition_index + 1, high_index, ascending);
    }
}
