#include "simple_scene.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/frame_data.h"
#include "core/ystring.h"
#include "data_structures/darray.h"
#include "math/transform.h"
#include "resources/resource_types.h"
#include "resources/skybox.h"
#include "resources/mesh.h"
#include "renderer/renderer_types.inl"
#include "systems/render_view_system.h"
#include "renderer/camera.h"
#include "math/ymath.h"
#include "systems/light_system.h"

static void simple_scene_actual_unload(SIMPLE_SCENE* scene);

static u32 global_scene_id = 0;

b8 simple_scene_create(void* config, SIMPLE_SCENE* out_scene) {
    if (!out_scene) {
        PRINT_ERROR(("simple_scene_create(): A valid pointer to out_scene is required."));
        return false;
    }

    yzero_memory(out_scene, sizeof(SIMPLE_SCENE));

    out_scene->enabled = false;
    out_scene->state = SIMPLE_SCENE_STATE_UNINITIALIZED;
    out_scene->scene_transform = transform_create();
    global_scene_id++;
    out_scene->id = global_scene_id;

    // Internal "lists" of renderable objects.
    out_scene->dir_light = 0;
    out_scene->point_lights = darray_create(POINT_LIGHT);
    out_scene->meshes = darray_create(Mesh);
    out_scene->sb = 0;

    if (config) {
        out_scene->config = yallocate_aligned(sizeof(SIMPLE_SCENE_CONFIG), 8, MEMORY_TAG_SCENE);
        ycopy_memory(out_scene->config, config, sizeof(SIMPLE_SCENE_CONFIG));
    }

    return true;
}

b8 simple_scene_init(SIMPLE_SCENE* scene) {
    if (!scene) {
        PRINT_ERROR("simple_scene_initialize requires a valid pointer to a scene.");
        return false;
    }

    // Process configuration and setup hierarchy.
    if (scene->config) {
        if (scene->config->name) {
            scene->name = string_duplicate(scene->config->name);
        }
        if (scene->config->description) {
            scene->description = string_duplicate(scene->config->description);
        }

        // Only setup a skybox if name and cubemap name are populated. Otherwise there isn't one.
        if (scene->config->skybox_config.name && scene->config->skybox_config.cubemap_name) {
            SKYBOX_CONFIG sb_config = {0};
            sb_config.cubemap_name = scene->config->skybox_config.cubemap_name;
            scene->sb = yallocate_aligned(sizeof(Skybox), 8, MEMORY_TAG_SCENE);
            if (!skybox_create(sb_config, scene->sb)) {
                PRINT_WARNING("Failed to create Skybox.");
                yfree(scene->sb);
                scene->sb = 0;
            }
        }

        // If no name is assigned, assume no directional light.
        if (scene->config->directional_light_config.name) {
            scene->dir_light = yallocate_aligned(sizeof(DIRECTIONAL_LIGHT), 8, MEMORY_TAG_SCENE);
            scene->dir_light->name = string_duplicate(scene->config->directional_light_config.name);
            scene->dir_light->data.color = scene->config->directional_light_config.color;
            scene->dir_light->data.direction = scene->config->directional_light_config.direction;
        }

        // Point lights.
        u32 p_light_count = darray_length(scene->config->point_lights);
        for (u32 i = 0; i < p_light_count; ++i) {
            POINT_LIGHT new_light = {0};
            new_light.name = string_duplicate(scene->config->point_lights[i].name);
            new_light.data.color = scene->config->point_lights[i].color;
            new_light.data.constant_f = scene->config->point_lights[i].constant_f;
            new_light.data.linear = scene->config->point_lights[i].linear;
            new_light.data.position = scene->config->point_lights[i].position;
            new_light.data.quadratic = scene->config->point_lights[i].quadratic;

            darray_push(scene->point_lights, new_light);
        }

        // Meshes
        u32 mesh_config_count = darray_length(scene->config->meshes);
        for (u32 i = 0; i < mesh_config_count; ++i) {
            if (!scene->config->meshes[i].name || !scene->config->meshes[i].resource_name) {
                PRINT_WARNING("Invalid mesh config, name and resource_name are required.");
                continue;
            }
            
            MESH_CONFIG new_mesh_config = {0};
            new_mesh_config.name = string_duplicate(scene->config->meshes[i].name);
            new_mesh_config.resource_name = string_duplicate(scene->config->meshes[i].resource_name);
            if (scene->config->meshes[i].parent_name) {
                new_mesh_config.parent_name = string_duplicate(scene->config->meshes[i].parent_name);
            }
            Mesh new_mesh = {0};
            if (!mesh_create(new_mesh_config, &new_mesh)) {
                PRINT_ERROR("Failed to new mesh in simple scene.");
                yfree(new_mesh_config.name);
                yfree(new_mesh_config.resource_name);
                if (new_mesh_config.parent_name) {
                    yfree(new_mesh_config.parent_name);
                }
                continue;
            }
            new_mesh.transform = scene->config->meshes[i].transform;

            darray_push(scene->meshes, new_mesh);
        }
    }

    // Now handle hierarchy.
    // NOTE: This only currently supports heirarchy of meshes.
    u32 mesh_count = darray_length(scene->meshes);
    for (u32 i = 0; i < mesh_count; ++i) {
        if (scene->meshes[i].config.parent_name) {
            Mesh* parent = simple_scene_mesh_get(scene, scene->meshes[i].config.parent_name);
            if (!parent && scene->meshes[i].config.name && scene->meshes[i].config.parent_name) {
                PRINT_WARNING("Mesh '%s' is configured to have a parent called '%s', but the parent does not exist.", scene->meshes[i].config.name, scene->meshes[i].config.parent_name);
                continue;
            }

            transform_set_parent(&scene->meshes[i].transform, &parent->transform);
        }
    }

    if (scene->sb) {
        if (!skybox_init(scene->sb)) {
            PRINT_ERROR("Skybox failed to initialize.");
            scene->sb = 0;
            return false;
        }
    }

    for (u32 i = 0; i < mesh_count; ++i) {
        if (!mesh_init(&scene->meshes[i])) {
            PRINT_ERROR("Mesh failed to initialize.");
            return false;
        }
    }

    // Update the state to show the scene is initialized.
    scene->state = SIMPLE_SCENE_STATE_INITIALIZED;

    return true;
}

b8 simple_scene_load(SIMPLE_SCENE* scene) {
    if (!scene) {
        return false;
    }

    // Update the state to show the scene is currently loading.
    scene->state = SIMPLE_SCENE_STATE_LOADING;

    if (scene->sb) {
        if (scene->sb->instance_id == INVALID_ID) {
            if (!skybox_load(scene->sb)) {
                PRINT_ERROR("Skybox failed to load.");
                scene->sb = 0;
                return false;
            }
        }
    }

    u32 mesh_count = darray_length(scene->meshes);
    for (u32 i = 0; i < mesh_count; ++i) {
        if (!mesh_load(&scene->meshes[i])) {
            PRINT_ERROR("Mesh failed to load.");
            return false;
        }
    }

    if (scene->dir_light) {
        if (!light_system_add_directional(scene->dir_light)) {
            PRINT_WARNING("Failed to add directional light to lighting system.");
        }
    }

    u32 point_light_count = darray_length(scene->point_lights);
    for (u32 i = 0; i < point_light_count; ++i) {
        if (!light_system_add_point(&scene->point_lights[i])) {
            PRINT_WARNING("Failed to add point light to lighting system.");
        }
    }

    // Update the state to show the scene is fully loaded.
    scene->state = SIMPLE_SCENE_STATE_LOADED;

    return true;
}

b8 simple_scene_unload(SIMPLE_SCENE* scene, b8 immediate) {
    if (!scene) {
        return false;
    }

    if (immediate) {
        scene->state = SIMPLE_SCENE_STATE_UNLOADING;
        simple_scene_actual_unload(scene);
        return true;
    }

    // Update the state to show the scene is currently unloading.
    scene->state = SIMPLE_SCENE_STATE_UNLOADING;
    return true;
}

b8 simple_scene_update(SIMPLE_SCENE* scene, const struct FRAME_DATA* p_frame_data) {
    if (!scene) {
        return false;
    }

    if (scene->state == SIMPLE_SCENE_STATE_UNLOADING) {
        simple_scene_actual_unload(scene);
    }

    return true;
}

b8 simple_scene_populate_render_packet(SIMPLE_SCENE* scene, struct Camera* current_camera, f32 aspect, struct FRAME_DATA* p_frame_data, struct RENDER_PACKET* packet) {
    if (!scene || !packet) {
        return false;
    }

    // TODO: cache this somewhere so that we don't have to check this every time.
    for (u32 i = 0; i < packet->view_count; ++i) {
        RENDER_VIEW_PACKET* view_packet = &packet->views[i];
        const RENDER_VIEW* view = view_packet->view;
        if (view->type == RENDERER_VIEW_KNOWN_TYPE_SKYBOX) {
            if (scene->sb) {
                // Skybox
                SKYBOX_PACKET_DATA skybox_data = {0};
                skybox_data.sb = scene->sb;
                if (!render_view_system_build_packet(view, p_frame_data->frame_allocator, &skybox_data, view_packet)) {
                    PRINT_ERROR("Failed to build packet for view 'skybox'.");
                    return false;
                }
            }
            break;
        }
    }

    for (u32 i = 0; i < packet->view_count; ++i) {
        RENDER_VIEW_PACKET* view_packet = &packet->views[i];
        const RENDER_VIEW* view = view_packet->view;
        if (view->type == RENDERER_VIEW_KNOWN_TYPE_WORLD) {
            // Update the frustum
            Vector3 forward = camera_forward(current_camera);
            Vector3 right = camera_right(current_camera);
            Vector3 up = camera_up(current_camera);
            // TODO: get camera fov, aspect, etc.
            Frustum f = frustum_create(&current_camera->position, &forward, &right, &up, aspect, deg_to_rad(45.0f), 0.1f, 1000.0f);

            // NOTE: starting at a reasonable default to avoid too many reallocs.
            // TODO: Use frame allocator.
            GEOMETRY_RENDER_DATA* world_geometries = darray_reserve(GEOMETRY_RENDER_DATA, 512);
            p_frame_data->drawn_mesh_count = 0;

            u32 mesh_count = darray_length(scene->meshes);
            for (u32 i = 0; i < mesh_count; ++i) {
                Mesh* m = &scene->meshes[i];
                if (m->generation != INVALID_ID_U8) {
                    Matrice4 model = transform_get_world(&m->transform);

                    for (u32 j = 0; j < m->geometry_count; ++j) {
                        GEOMETRY* g = m->geometries[j];

                        // // Bounding sphere calculation.
                        // {
                        //     // Translate/scale the extents.
                        //     Vector3 extents_min = Vector3_multiply_Matrice4(g->extents.min, model);
                        //     Vector3 extents_max = Vector3_multiply_Matrice4(g->extents.max, model);

                        //     f32 min = KMIN(KMIN(extents_min.x, extents_min.y), extents_min.z);
                        //     f32 max = KMAX(KMAX(extents_max.x, extents_max.y), extents_max.z);
                        //     f32 diff = yabs(max - min);
                        //     f32 radius = diff * 0.5f;

                        //     // Translate/scale the center.
                        //     Vector3 center = Vector3_multiply_Matrice4(g->center, model);

                        //     if (frustum_intersects_sphere(&state->camera_frustum, &center, radius)) {
                        //         // Add it to the list to be rendered.
                        //         GEOMETRY_RENDER_DATA data = {0};
                        //         data.model = model;
                        //         data.GEOMETRY = g;
                        //         data.unique_id = m->unique_id;
                        //         darray_push(game_inst->frame_data.world_geometries, data);

                        //         draw_count++;
                        //     }
                        // }

                        // AABB calculation
                        {
                            // Translate/scale the extents.
                            // Vector3 extents_min = Vector3_multiply_Matrice4(g->extents.min, model);
                            Vector3 extents_max = Vector3_multiply_Matrice4(g->extents.max, model);

                            // Translate/scale the center.
                            Vector3 center = Vector3_multiply_Matrice4(g->center, model);
                            Vector3 half_extents = {
                                yabs(extents_max.x - center.x),
                                yabs(extents_max.y - center.y),
                                yabs(extents_max.z - center.z),
                            };

                            if (frustum_intersects_aabb(&f, &center, &half_extents)) {
                                // Add it to the list to be rendered.
                                GEOMETRY_RENDER_DATA data = {0};
                                data.model = model;
                                data.geometry = g;
                                data.unique_id = m->unique_id;
                                darray_push(world_geometries, data);

                                p_frame_data->drawn_mesh_count++;
                            }
                        }
                    }
                }
            }

            // World
            if (!render_view_system_build_packet(render_view_system_get("world"), p_frame_data->frame_allocator, world_geometries, &packet->views[1])) {
                PRINT_ERROR("Failed to build packet for view 'world_opaque'.");
                return false;
            }

            // TODO: bad.....
            darray_destroy(world_geometries);
        }
    }

    return true;
}

b8 simple_scene_add_directional_light(SIMPLE_SCENE* scene, const char* name, struct DIRECTIONAL_LIGHT* light) {
    if (!scene) {
        return false;
    }

    if (scene->dir_light) {
        // TODO: Do any resource unloading required.
        light_system_remove_directional(scene->dir_light);
    }

    if (light) {
        if (!light_system_add_directional(light)) {
            PRINT_ERROR("simple_scene_add_directional_light - failed to add directional light to light system.");
            return false;
        }
    }

    scene->dir_light = light;

    return true;
}

b8 simple_scene_add_point_light(SIMPLE_SCENE* scene, const char* name, struct POINT_LIGHT* light) {
    if (!scene || !light) {
        return false;
    }

    if (!light_system_add_point(light)) {
        PRINT_ERROR("Failed to add point light to scene (light system add failure, check logs).");
        return false;
    }

    darray_push(scene->point_lights, light);

    return true;
}

b8 simple_scene_add_mesh(SIMPLE_SCENE* scene, const char* name, struct Mesh* m) {
    if (!scene || !m) {
        return false;
    }

    if (scene->state > SIMPLE_SCENE_STATE_INITIALIZED) {
        if (!mesh_init(m)) {
            PRINT_ERROR("Mesh failed to initialize.");
            return false;
        }
    }

    if (scene->state >= SIMPLE_SCENE_STATE_LOADED) {
        if (!mesh_load(m)) {
            PRINT_ERROR("Mesh failed to load.");
            return false;
        }
    }

    darray_push(scene->meshes, m);

    return true;
}

b8 simple_scene_add_skybox(SIMPLE_SCENE* scene, const char* name, struct Skybox* sb) {
    if (!scene) {
        return false;
    }

    // TODO: if one already exists, do we do anything with it?
    scene->sb = sb;
    if (scene->state > SIMPLE_SCENE_STATE_INITIALIZED) {
        if (!skybox_init(sb)) {
            PRINT_ERROR("Skybox failed to initialize.");
            scene->sb = 0;
            return false;
        }
    }

    if (scene->state >= SIMPLE_SCENE_STATE_LOADED) {
        if (!skybox_load(sb)) {
            PRINT_ERROR("Skybox failed to load.");
            scene->sb = 0;
            return false;
        }
    }

    return true;
}

b8 simple_scene_remove_directional_light(SIMPLE_SCENE* scene, const char* name) {
    if (!scene || !name) {
        return false;
    }

    if (!scene->dir_light || !strings_equal(scene->dir_light->name, name)) {
        PRINT_WARNING("Cannot remove directional light from scene that is not part of the scene.");
        return false;
    }

    if (!light_system_remove_directional(scene->dir_light)) {
        PRINT_ERROR("Failed to remove directional light from light system.");
        return false;
    }

    yfree(scene->dir_light);
    scene->dir_light = 0;

    return true;
}

b8 simple_scene_remove_point_light(SIMPLE_SCENE* scene, const char* name) {
    if (!scene || !name) {
        return false;
    }

    u32 light_count = darray_length(scene->point_lights);
    for (u32 i = 0; i < light_count; ++i) {
        if (strings_equal(scene->point_lights[i].name, name)) {
            if (!light_system_remove_point(&scene->point_lights[i])) {
                PRINT_ERROR("Failed to remove point light from light system.");
                return false;
            }

            POINT_LIGHT rubbish = {0};
            darray_pop_at(scene->point_lights, i, &rubbish);

            return true;
        }
    }

    PRINT_ERROR("Cannot remove a point light from a scene of which it is not a part.");
    return false;
}

b8 simple_scene_remove_mesh(SIMPLE_SCENE* scene, const char* name) {
    if (!scene || !name) {
        return false;
    }

    u32 mesh_count = darray_length(scene->meshes);
    for (u32 i = 0; i < mesh_count; ++i) {
        if (strings_equal(scene->meshes[i].name, name)) {
            if (!mesh_unload(&scene->meshes[i])) {
                PRINT_ERROR("Failed to unload mesh");
                return false;
            }

            Mesh rubbish = {0};
            darray_pop_at(scene->meshes, i, &rubbish);

            return true;
        }
    }

    PRINT_ERROR("Cannot remove a point light from a scene of which it is not a part.");
    return false;
}

b8 simple_scene_remove_skybox(SIMPLE_SCENE* scene, const char* name) {
    if (!scene || !name) {
        return false;
    }

    // TODO: name?
    if (!scene->sb) {
        PRINT_WARNING("Cannot remove skybox from a scene of which it is not a part.");
        return false;
    }

    scene->sb = 0;

    return true;
}

struct DIRECTIONAL_LIGHT* simple_scene_directional_light_get(SIMPLE_SCENE* scene, const char* name) {
    if (!scene) {
        return 0;
    }

    return scene->dir_light;
}

struct POINT_LIGHT* simple_scene_point_light_get(SIMPLE_SCENE* scene, const char* name) {
    if (!scene) {
        return 0;
    }

    u32 length = darray_length(scene->point_lights);
    for (u32 i = 0; i < length; ++i) {
        if (strings_nequal(name, scene->point_lights[i].name, 256)) {
            return &scene->point_lights[i];
        }
    }

    PRINT_WARNING("Simple scene does not contain a point light called '%s'.", name);
    return 0;
}

struct Mesh* simple_scene_mesh_get(SIMPLE_SCENE* scene, const char* name) {
    if (!scene) {
        return 0;
    }

    u32 length = darray_length(scene->meshes);
    for (u32 i = 0; i < length; ++i) {
        if (strings_nequal(name, scene->meshes[i].name, 256)) {
            return &scene->meshes[i];
        }
    }

    PRINT_WARNING("Simple scene does not contain a mesh called '%s'.", name);
    return 0;
}

struct Skybox* simple_scene_skybox_get(SIMPLE_SCENE* scene, const char* name) {
    if (!scene) {
        return 0;
    }

    return scene->sb;
}

static void simple_scene_actual_unload(SIMPLE_SCENE* scene) {
    if (scene->sb) {
        if (!skybox_unload(scene->sb)) {
            PRINT_ERROR("Failed to unload Skybox");
        }
        skybox_destroy(scene->sb);
        scene->sb = 0;
    }

    u32 mesh_count = darray_length(scene->meshes);
    for (u32 i = 0; i < mesh_count; ++i) {
        if (scene->meshes[i].generation != INVALID_ID_U8) {
            if (!mesh_unload(&scene->meshes[i])) {
                PRINT_ERROR("Failed to unload Mesh.");
            }
        }
    }

    if (scene->dir_light) {
        // TODO: If there are resource to unload, that should be done before this next line. Ex: box representing pos/color
        if (!simple_scene_remove_directional_light(scene, scene->dir_light->name)) {
            PRINT_ERROR("Failed to unload/remove directional light.");
        }
    }

    u32 p_light_count = darray_length(scene->point_lights);
    for (u32 i = 0; i < p_light_count; ++i) {
        if (!light_system_remove_point(&scene->point_lights[i])) {
            PRINT_WARNING("Failed to remove point light from light system.");
        }

        // TODO: If there are resource to unload, that should be done before this next line. Ex: box representing pos/color
    }

    // Update the state to show the scene is initialized.
    scene->state = SIMPLE_SCENE_STATE_UNLOADED;

    // Also destroy the scene.
    scene->dir_light = 0;
    scene->sb = 0;

    if (scene->point_lights) {
        darray_destroy(scene->point_lights);
    }

    if (scene->meshes) {
        darray_destroy(scene->meshes);
    }

    yzero_memory(scene, sizeof(SIMPLE_SCENE));
}
