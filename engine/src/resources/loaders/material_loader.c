#include "material_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/ymath.h"
#include "loader_utils.h"

#include "io/filesystem.h"

b8 material_loader_load(RESOURCE_LOADER* self, const char* name, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".y_material");

    FILE_HANDLE f;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, false, &f)) {
        PRINT_ERROR("material_loader_load - unable to open material file for reading: '%s'.", full_file_path);
        return false;
    }

    // TODO: Should be using an allocator here.
    out_resource->full_path = string_duplicate(full_file_path);

    // TODO: Should be using an allocator here.
    MATERIAL_CONFIG* resource_data = yallocate_aligned(sizeof(MATERIAL_CONFIG), 8, MEMORY_TAG_MATERIAL_INSTANCE);
    // Set some defaults.
    resource_data->shader_name = "builtin.material"; // default material
    resource_data->auto_release = true;
    resource_data->diffuse_colour = Vector4_one();  // white.
    resource_data->diffuse_map_name[0] = 0;
    string_ncopy(resource_data->name, name, MATERIAL_NAME_MAX_LENGTH);

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
        if (strings_equali(trimmed_var_name, "version")) {
            // TODO: version
        } else if (strings_equali(trimmed_var_name, "name")) {
            string_ncopy(resource_data->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(resource_data->diffuse_map_name, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_colour")) {
            // Parse the colour
            if (!string_to_vector4(trimmed_value, &resource_data->diffuse_colour)) {
                PRINT_WARNING("Error parsing diffuse_colour in file '%s'. Using default of white instead.", full_file_path);
                // NOTE: already assigned above, no need to have it here.
            }
        } else if (strings_equali(trimmed_var_name, "shader")) {
            // Take a copy of the material name.
            resource_data->shader_name = string_duplicate(trimmed_value);
        }

        // TODO: more fields.

        // Clear the line buffer.
        yzero_memory(line_buf, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(&f);

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(MATERIAL_CONFIG);
    out_resource->name = name;

    return true;
}

void material_loader_unload(RESOURCE_LOADER* self, RESOURCE* resource) {
    if (!resource_unload(self, resource, MEMORY_TAG_MATERIAL_INSTANCE)) {
        PRINT_WARNING("material_loader_unload called with nullptr for self or resource.");
        return;
    }
}

RESOURCE_LOADER material_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_MATERIAL;
    loader.custom_type = 0;
    loader.load = material_loader_load;
    loader.unload = material_loader_unload;
    loader.type_path = "materials";

    return loader;
}
