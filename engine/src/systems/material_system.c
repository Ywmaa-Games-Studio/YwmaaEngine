#include "material_system.h"

#include "core/logger.h"
#include "core/ystring.h"
#include "variants/hashtable.h"
#include "math/ymath.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

// TODO: temp: resource system
#include "io/filesystem.h"
// end temp

typedef struct MATERIAL_SYSTEM_STATE {
    MATERIAL_SYSTEM_CONFIG config;

    MATERIAL default_material;

    // Array of registered materials.
    MATERIAL* registered_materials;

    // Hashtable for material lookups.
    HASHTABLE registered_material_table;
} MATERIAL_SYSTEM_STATE;

typedef struct MATERIAL_REFERENCE {
    u64 reference_count;
    u32 handle;
    b8 auto_release;
} MATERIAL_REFERENCE;

static MATERIAL_SYSTEM_STATE* state_ptr = 0;

b8 create_default_material(MATERIAL_SYSTEM_STATE* state);
b8 load_material(MATERIAL_CONFIG config, MATERIAL* m);
void destroy_material(MATERIAL* m);
b8 load_configuration_file(const char* path, MATERIAL_CONFIG* out_config);

b8 material_system_init(u64* memory_requirement, void* state, MATERIAL_SYSTEM_CONFIG config) {
    if (config.max_material_count == 0) {
        PRINT_ERROR("material_system_initialize - config.max_material_count must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 struct_requirement = sizeof(MATERIAL_SYSTEM_STATE);
    u64 array_requirement = sizeof(MATERIAL) * config.max_material_count;
    u64 hashtable_requirement = sizeof(MATERIAL_REFERENCE) * config.max_material_count;
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

    if (!state) {
        return true;
    }

    state_ptr = state;
    state_ptr->config = config;

    // The array block is after the state. Already allocated, so just set the pointer.
    void* array_block = state + struct_requirement;
    state_ptr->registered_materials = array_block;

    // Hashtable block is after array.
    void* hashtable_block = array_block + array_requirement;

    // Create a hashtable for MATERIAL lookups.
    hashtable_create(sizeof(MATERIAL_REFERENCE), config.max_material_count, hashtable_block, false, &state_ptr->registered_material_table);

    // Fill the hashtable with invalid references to use as a default.
    MATERIAL_REFERENCE invalid_ref;
    invalid_ref.auto_release = false;
    invalid_ref.handle = INVALID_ID;  // Primary reason for needing default values.
    invalid_ref.reference_count = 0;
    hashtable_fill(&state_ptr->registered_material_table, &invalid_ref);

    // Invalidate all materials in the array.
    u32 count = state_ptr->config.max_material_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->registered_materials[i].id = INVALID_ID;
        state_ptr->registered_materials[i].generation = INVALID_ID;
        state_ptr->registered_materials[i].internal_id = INVALID_ID;
    }

    if (!create_default_material(state_ptr)) {
        PRINT_ERROR("Failed to create default material. Application cannot continue.");
        return false;
    }

    return true;
}

void material_system_shutdown(void* state) {
    MATERIAL_SYSTEM_STATE* s = (MATERIAL_SYSTEM_STATE*)state;
    if (s) {
        // Invalidate all materials in the array.
        u32 count = s->config.max_material_count;
        for (u32 i = 0; i < count; ++i) {
            if (s->registered_materials[i].id != INVALID_ID) {
                destroy_material(&s->registered_materials[i]);
            }
        }

        // Destroy the default material.
        destroy_material(&s->default_material);
    }

    state_ptr = 0;
}

MATERIAL* material_system_acquire(const char* name) {
    // Load the given material configuration from disk.
    MATERIAL_CONFIG config;
    config.auto_release = false;

    // Load file from disk
    // TODO: Should be able to be located anywhere.
    char* format_str = "assets/materials/%s.%s";
    char full_file_path[512];

    // TODO: try different extensions
    string_format(full_file_path, format_str, name, "y_material");
    if (!load_configuration_file(full_file_path, &config)) {
        PRINT_ERROR("Failed to load material file: '%s'. Null pointer will be returned.", full_file_path);
        return 0;
    }

    // Now acquire from loaded config.
    return material_system_acquire_from_config(config);
}

MATERIAL* material_system_acquire_from_config(MATERIAL_CONFIG config) {
    // Return default material.
    if (strings_equali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &state_ptr->default_material;
    }

    MATERIAL_REFERENCE ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_material_table, config.name, &ref)) {
        // This can only be changed the first time a material is loaded.
        if (ref.reference_count == 0) {
            ref.auto_release = config.auto_release;
        }
        ref.reference_count++;
        if (ref.handle == INVALID_ID) {
            // This means no material exists here. Find a free index first.
            u32 count = state_ptr->config.max_material_count;
            MATERIAL* m = 0;
            for (u32 i = 0; i < count; ++i) {
                if (state_ptr->registered_materials[i].id == INVALID_ID) {
                    // A free slot has been found. Use its index as the handle.
                    ref.handle = i;
                    m = &state_ptr->registered_materials[i];
                    break;
                }
            }

            // Make sure an empty slot was actually found.
            if (!m || ref.handle == INVALID_ID) {
                PRINT_ERROR("material_system_acquire - Material system cannot hold anymore materials. Adjust configuration to allow more.");
                return 0;
            }

            // Create new material.
            if (!load_material(config, m)) {
                PRINT_ERROR("Failed to load material '%s'.", config.name);
                return 0;
            }

            if (m->generation == INVALID_ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            // Also use the handle as the material id.
            m->id = ref.handle;
            PRINT_DEBUG("Material '%s' does not yet exist. Created, and ref_count is now %i.", config.name, ref.reference_count);
        } else {
            PRINT_DEBUG("Material '%s' already exists, ref_count increased to %i.", config.name, ref.reference_count);
        }

        // Update the entry.
        hashtable_set(&state_ptr->registered_material_table, config.name, &ref);
        return &state_ptr->registered_materials[ref.handle];
    }

    // NOTE: This would only happen in the event something went wrong with the state.
    PRINT_ERROR("material_system_acquire_from_config failed to acquire material '%s'. Null pointer will be returned.", config.name);
    return 0;
}

void material_system_release(const char* name) {
    // Ignore release requests for the default material.
    if (strings_equali(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }
    MATERIAL_REFERENCE ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_material_table, name, &ref)) {
        if (ref.reference_count == 0) {
            PRINT_WARNING("Tried to release non-existent material: '%s'", name);
            return;
        }
        ref.reference_count--;
        if (ref.reference_count == 0 && ref.auto_release) {
            MATERIAL* m = &state_ptr->registered_materials[ref.handle];

            // Destroy/reset material.
            destroy_material(m);

            // Reset the reference.
            ref.handle = INVALID_ID;
            ref.auto_release = false;
            PRINT_DEBUG("Released material '%s'., Material unloaded because reference count=0 and auto_release=true.", name);
        } else {
            PRINT_DEBUG("Released material '%s', now has a reference count of '%i' (auto_release=%s).", name, ref.reference_count, ref.auto_release ? "true" : "false");
        }

        // Update the entry.
        hashtable_set(&state_ptr->registered_material_table, name, &ref);
    } else {
        PRINT_ERROR("material_system_release failed to release material '%s'.", name);
    }
}

MATERIAL* material_system_get_default(void) {
    if (state_ptr) {
        return &state_ptr->default_material;
    }

    PRINT_ERROR("material_system_get_default called before system is initialized.");
    return 0;
}


b8 load_material(MATERIAL_CONFIG config, MATERIAL* m) {
    yzero_memory(m, sizeof(MATERIAL));

    // name
    string_ncopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    // Diffuse colour
    m->diffuse_colour = config.diffuse_colour;

    // Diffuse map
    if (string_length(config.diffuse_map_name) > 0) {
        m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuse_map.texture = texture_system_acquire(config.diffuse_map_name, true);
        if (!m->diffuse_map.texture) {
            PRINT_WARNING("Unable to load texture '%s' for material '%s', using default.", config.diffuse_map_name, m->name);
            m->diffuse_map.texture = texture_system_get_default_texture();
        }
    } else {
        // NOTE: Only set for clarity, as call to yzero_memory above does this already.
        m->diffuse_map.use = TEXTURE_USE_UNKNOWN;
        m->diffuse_map.texture = 0;
    }

    // TODO: other maps

    // Send it off to the renderer to acquire resources.
    if (!renderer_create_material(m)) {
        PRINT_ERROR("Failed to acquire renderer resources for material '%s'.", m->name);
        return false;
    }

    return true;
}

void destroy_material(MATERIAL* m) {
    PRINT_DEBUG("Destroying material '%s'...", m->name);

    // Release texture references.
    if (m->diffuse_map.texture) {
        texture_system_release(m->diffuse_map.texture->name);
    }

    // Release renderer resources.
    renderer_destroy_material(m);

    // Zero it out, invalidate IDs.
    yzero_memory(m, sizeof(MATERIAL));
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internal_id = INVALID_ID;
}

b8 create_default_material(MATERIAL_SYSTEM_STATE* state) {
    yzero_memory(&state->default_material, sizeof(MATERIAL));
    state->default_material.id = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    string_ncopy(state->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->default_material.diffuse_colour = Vector4_one();  // white
    state->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    state->default_material.diffuse_map.texture = texture_system_get_default_texture();

    if (!renderer_create_material(&state->default_material)) {
        PRINT_ERROR("Failed to acquire renderer resources for default texture. Application cannot continue.");
        return false;
    }

    return true;
}

b8 load_configuration_file(const char* path, MATERIAL_CONFIG* out_config) {
    FILE_HANDLE f;
    if (!filesystem_open(path, FILE_MODE_READ, false, &f)) {
        PRINT_ERROR("load_configuration_file - unable to open material file for reading: '%s'.", path);
        return false;
    }

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
            PRINT_WARNING("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", path, line_number);
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
            string_ncopy(out_config->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(out_config->diffuse_map_name, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_colour")) {
            // Parse the colour
            if (!string_to_vector4(trimmed_value, &out_config->diffuse_colour)) {
                PRINT_WARNING("Error parsing diffuse_colour in file '%s'. Using default of white instead.", path);
                out_config->diffuse_colour = Vector4_one();  // white
            }
        }

        // TODO: more fields.

        // Clear the line buffer.
        yzero_memory(line_buf, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(&f);

    return true;
}
