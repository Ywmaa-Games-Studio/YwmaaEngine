#include "simple_scene_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/ymath.h"
#include "math/transform.h"
#include "loader_utils.h"
#include "data_structures/darray.h"

#include "io/filesystem.h"

typedef enum E_SIMPLE_SCENE_PARSE_MODE {
    SIMPLE_SCENE_PARSE_MODE_ROOT,
    SIMPLE_SCENE_PARSE_MODE_SCENE,
    SIMPLE_SCENE_PARSE_MODE_SKYBOX,
    SIMPLE_SCENE_PARSE_MODE_DIRECTIONAL_LIGHT,
    SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT,
    SIMPLE_SCENE_PARSE_MODE_MESH
} E_SIMPLE_SCENE_PARSE_MODE;

static b8 try_change_mode(const char* value, E_SIMPLE_SCENE_PARSE_MODE* current, E_SIMPLE_SCENE_PARSE_MODE expected_current, E_SIMPLE_SCENE_PARSE_MODE target);

b8 simple_scene_loader_load(struct RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".y_simple_scene");

    FILE_HANDLE f;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, false, &f)) {
        PRINT_ERROR("simple_scene_loader_load - unable to open simple scene file for reading: '%s'.", full_file_path);
        return false;
    }

    out_resource->full_path = string_duplicate(full_file_path);

    SIMPLE_SCENE_CONFIG* resource_data = yallocate_aligned(sizeof(SIMPLE_SCENE_CONFIG), 8, MEMORY_TAG_RESOURCE);
    yzero_memory(resource_data, sizeof(SIMPLE_SCENE_CONFIG));

    // Set some defaults, create arrays.
    resource_data->description = 0;
    resource_data->name = string_duplicate(name);
    resource_data->point_lights = darray_create(POINT_LIGHT_SIMPLE_SCENE_CONFIG);
    resource_data->meshes = darray_create(MESH_SIMPLE_SCENE_CONFIG);

    u32 version = 0;
    E_SIMPLE_SCENE_PARSE_MODE mode = SIMPLE_SCENE_PARSE_MODE_ROOT;

    // Buffer objects that get populated when in corresponding mode, and pushed to list when
    // leaving said mode.
    POINT_LIGHT_SIMPLE_SCENE_CONFIG current_point_light_config = {0};
    MESH_SIMPLE_SCENE_CONFIG current_mesh_config = {0};

    // Read each line of the file.
    char line_buf[512] = "";
    char* p = &line_buf[0];
    u64 line_length = 0;
    u32 line_number = 1;
    while (filesystem_read_line(&f, 511, &p, &line_length)) {
        // Trim the string.
        char* trimmed = string_trim(line_buf);

        // Get the trimmed length.
        line_length = string_length(trimmed);

        // Skip blank lines and comments.
        if (line_length < 1 || trimmed[0] == '#') {
            line_number++;
            continue;
        }

        if (trimmed[0] == '[') {
            if (version == 0) {
                PRINT_ERROR("Error loading simple scene file, !version was not set before attempting to change modes.");
                return false;
            }

            // Change modes
            if (strings_equali(trimmed, "[Scene]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_ROOT, SIMPLE_SCENE_PARSE_MODE_SCENE)) {
                    return false;
                }
            } else if (strings_equali(trimmed, "[/Scene]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_SCENE, SIMPLE_SCENE_PARSE_MODE_ROOT)) {
                    return false;
                }
            } else if (strings_equali(trimmed, "[Skybox]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_ROOT, SIMPLE_SCENE_PARSE_MODE_SKYBOX)) {
                    return false;
                }
            } else if (strings_equali(trimmed, "[/Skybox]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_SKYBOX, SIMPLE_SCENE_PARSE_MODE_ROOT)) {
                    return false;
                }
            } else if (strings_equali(trimmed, "[DirectionalLight]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_ROOT, SIMPLE_SCENE_PARSE_MODE_DIRECTIONAL_LIGHT)) {
                    return false;
                }
            } else if (strings_equali(trimmed, "[/DirectionalLight]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_DIRECTIONAL_LIGHT, SIMPLE_SCENE_PARSE_MODE_ROOT)) {
                    return false;
                }
            } else if (strings_equali(trimmed, "[PointLight]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_ROOT, SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT)) {
                    return false;
                }
                yzero_memory(&current_point_light_config, sizeof(POINT_LIGHT_SIMPLE_SCENE_CONFIG));
            } else if (strings_equali(trimmed, "[/PointLight]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT, SIMPLE_SCENE_PARSE_MODE_ROOT)) {
                    return false;
                }
                // Push into the array, then cleanup.
                darray_push(resource_data->point_lights, current_point_light_config);
                yzero_memory(&current_point_light_config, sizeof(POINT_LIGHT_SIMPLE_SCENE_CONFIG));
            } else if (strings_equali(trimmed, "[Mesh]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_ROOT, SIMPLE_SCENE_PARSE_MODE_MESH)) {
                    return false;
                }
                yzero_memory(&current_mesh_config, sizeof(MESH_SIMPLE_SCENE_CONFIG));
                // Also setup a default transform.
                current_mesh_config.transform = transform_create();
            } else if (strings_equali(trimmed, "[/Mesh]")) {
                if (!try_change_mode(trimmed, &mode, SIMPLE_SCENE_PARSE_MODE_MESH, SIMPLE_SCENE_PARSE_MODE_ROOT)) {
                    return false;
                }
                if (!current_mesh_config.name || !current_mesh_config.resource_name) {
                    PRINT_WARNING("Format error: meshes require both name and RESOURCE name. Mesh not added.");
                    continue;
                }
                // Push into the array, then cleanup.
                darray_push(resource_data->meshes, current_mesh_config);
                yzero_memory(&current_mesh_config, sizeof(MESH_SIMPLE_SCENE_CONFIG));
            } else {
                PRINT_ERROR("Error loading simple scene file: format error. Unexpected object type '%s'", trimmed);
                return false;
            }
        } else {
            // Split into var/value
            i32 equal_index = string_index_of(trimmed, '=');
            if (equal_index == -1) {
                PRINT_WARNING("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", full_file_path, line_number);
                line_number++;
                continue;
            }

            // Assume a max of 64 characters for the variable name.
            char raw_var_name[64];
            yzero_memory(raw_var_name, sizeof(char) * 64);
            string_mid(raw_var_name, trimmed, 0, equal_index);
            char* trimmed_var_name = string_trim(raw_var_name);

            // Assume a max of 511-65 (446) for the max length of the value to account for the variable name and the '='.
            char raw_value[446];
            yzero_memory(raw_value, sizeof(char) * 446);
            string_mid(raw_value, trimmed, equal_index + 1, -1);  // Read the rest of the line
            char* trimmed_value = string_trim(raw_value);

            // Process the variable.
            if (strings_equali(trimmed_var_name, "!version")) {
                if (mode != SIMPLE_SCENE_PARSE_MODE_ROOT) {
                    PRINT_ERROR("Attempting to set version inside of non-root mode.");
                    return false;
                }
                if (!string_to_u32(trimmed_value, &version)) {
                    PRINT_ERROR("Invalid value for version: %s", trimmed_value);
                    // TODO: cleanup config
                    return false;
                }
            } else if (strings_equali(trimmed_var_name, "name")) {
                switch (mode) {
                    default:
                    case SIMPLE_SCENE_PARSE_MODE_ROOT:
                        PRINT_WARNING("Format warning: Cannot process name in root node.");
                        break;
                    case SIMPLE_SCENE_PARSE_MODE_SCENE:
                        resource_data->name = string_duplicate(trimmed_value);
                        break;
                    case SIMPLE_SCENE_PARSE_MODE_DIRECTIONAL_LIGHT:
                        resource_data->directional_light_config.name = string_duplicate(trimmed_value);
                        break;
                    case SIMPLE_SCENE_PARSE_MODE_SKYBOX:
                        resource_data->skybox_config.name = string_duplicate(trimmed_value);
                        break;
                    case SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT: {
                        current_point_light_config.name = string_duplicate(trimmed_value);
                    } break;
                    case SIMPLE_SCENE_PARSE_MODE_MESH:
                        current_mesh_config.name = string_duplicate(trimmed_value);
                        break;
                }
            } else if (strings_equali(trimmed_var_name, "color")) {
                switch (mode) {
                    default:
                        PRINT_WARNING("Format warning: Cannot process name in the current node.");
                        break;
                    case SIMPLE_SCENE_PARSE_MODE_DIRECTIONAL_LIGHT:
                        if (!string_to_vector4(trimmed_value, &resource_data->directional_light_config.color)) {
                            PRINT_WARNING("Format error parsing color. Default value used.");
                            resource_data->directional_light_config.color = Vector4_one();
                        }
                        break;
                    case SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT:
                        if (!string_to_vector4(trimmed_value, &current_point_light_config.color)) {
                            PRINT_WARNING("Format error parsing color. Default value used.");
                            current_point_light_config.color = Vector4_one();
                        }
                        break;
                }
            } else if (strings_equali(trimmed_var_name, "description")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_SCENE) {
                    resource_data->description = string_duplicate(trimmed_value);
                } else {
                    PRINT_WARNING("Format warning: Cannot process description in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "cubemap_name")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_SKYBOX) {
                    resource_data->skybox_config.cubemap_name = string_duplicate(trimmed_value);
                } else {
                    PRINT_WARNING("Format warning: Cannot process cubemap_name in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "resource_name")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_MESH) {
                    current_mesh_config.resource_name = string_duplicate(trimmed_value);
                } else {
                    PRINT_WARNING("Format warning: Cannot process resource_name in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "parent")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_MESH) {
                    current_mesh_config.parent_name = string_duplicate(trimmed_value);
                } else {
                    PRINT_WARNING("Format warning: Cannot process resource_name in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "direction")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_DIRECTIONAL_LIGHT) {
                    if (!string_to_vector4(trimmed_value, &resource_data->directional_light_config.direction)) {
                        PRINT_WARNING("Error parsing directional light direction as Vector4. Using default value");
                        resource_data->directional_light_config.direction = (Vector4){-0.57735f, -0.57735f, -0.57735f, 0.0f};
                    }
                } else {
                    PRINT_WARNING("Format warning: Cannot process direction in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "position")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT) {
                    if (!string_to_vector4(trimmed_value, &current_point_light_config.position)) {
                        PRINT_WARNING("Error parsing point light position as Vector4. Using default value");
                        current_point_light_config.position = Vector4_zero();
                    }
                } else {
                    PRINT_WARNING("Format warning: Cannot process direction in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "transform")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_MESH) {
                    if (!string_to_transform(trimmed_value, &current_mesh_config.transform)) {
                        PRINT_WARNING("Error parsing mesh transform. Using default value.");
                    }
                } else {
                    PRINT_WARNING("Format warning: Cannot process transform in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "constant_f")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT) {
                    if (!string_to_f32(trimmed_value, &current_point_light_config.constant_f)) {
                        PRINT_WARNING("Error parsing point light constant_f. Using default value.");
                        current_point_light_config.constant_f = 1.0f;
                    }
                } else {
                    PRINT_WARNING("Format warning: Cannot process constant in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "linear")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT) {
                    if (!string_to_f32(trimmed_value, &current_point_light_config.linear)) {
                        PRINT_WARNING("Error parsing point light linear. Using default value.");
                        current_point_light_config.linear = 0.35f;
                    }
                } else {
                    PRINT_WARNING("Format warning: Cannot process linear in the current node.");
                }
            } else if (strings_equali(trimmed_var_name, "quadratic")) {
                if (mode == SIMPLE_SCENE_PARSE_MODE_POINT_LIGHT) {
                    if (!string_to_f32(trimmed_value, &current_point_light_config.quadratic)) {
                        PRINT_WARNING("Error parsing point light quadratic. Using default value.");
                        current_point_light_config.quadratic = 0.44f;
                    }
                } else {
                    PRINT_WARNING("Format warning: Cannot process quadratic in the current node.");
                }
            }
        }

        // TODO: more fields.

        // Clear the line buffer.
        yzero_memory(line_buf, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(&f);

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(SIMPLE_SCENE_CONFIG);

    return true;
}

void simple_scene_loader_unload(struct RESOURCE_LOADER* self, RESOURCE* RESOURCE) {
    SIMPLE_SCENE_CONFIG* data = (SIMPLE_SCENE_CONFIG*)RESOURCE->data;

    if (data->meshes) {
        u32 length = darray_length(data->meshes);
        for (u32 i = 0; i < length; ++i) {
            if (data->meshes[i].name) {
                yfree(data->meshes[i].name);
            }
            if (data->meshes[i].parent_name) {
                yfree(data->meshes[i].parent_name);
            }
            if (data->meshes[i].resource_name) {
                yfree(data->meshes[i].resource_name);
            }
        }
        darray_destroy(data->meshes);
    }
    if (data->point_lights) {
        u32 length = darray_length(data->point_lights);
        for (u32 i = 0; i < length; ++i) {
            if (data->point_lights[i].name) {
                yfree(data->point_lights[i].name);
            }
        }
        darray_destroy(data->point_lights);
    }

    if (data->directional_light_config.name) {
        yfree(data->directional_light_config.name);
    }

    if (data->skybox_config.name) {
        yfree(data->skybox_config.name);
    }
    if (data->skybox_config.cubemap_name) {
        yfree(data->skybox_config.cubemap_name);
    }

    if (data->name) {
        yfree(data->name);
    }

    if (data->description) {
        yfree(data->description);
    }

    if (!resource_unload(self, RESOURCE, MEMORY_TAG_RESOURCE)) {
        PRINT_WARNING("simple_scene_loader_unload called with nullptr for self or RESOURCE.");
    }
}

RESOURCE_LOADER simple_scene_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_SIMPLE_SCENE;
    loader.custom_type = 0;
    loader.load = simple_scene_loader_load;
    loader.unload = simple_scene_loader_unload;
    loader.type_path = "scenes";

    return loader;
}

static b8 try_change_mode(const char* value, E_SIMPLE_SCENE_PARSE_MODE* current, E_SIMPLE_SCENE_PARSE_MODE expected_current, E_SIMPLE_SCENE_PARSE_MODE target) {
    if (*current != expected_current) {
        PRINT_ERROR("Error loading simple scene: format error. Unexpected token '%'", value);
        return false;
    } else {
        *current = target;
        return true;
    }
}
