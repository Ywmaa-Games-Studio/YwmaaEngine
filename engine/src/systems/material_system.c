#include "material_system.h"

#include "core/logger.h"
#include "core/ystring.h"
#include "variants/hashtable.h"
#include "math/ymath.h"
#include "renderer/renderer_frontend.h"

#include "systems/texture_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

typedef struct MATERIAL_SHADER_UNIFORM_LOCATIONS {
    u16 projection;
    u16 view;
    u16 ambient_colour;
    u16 diffuse_colour;
    u16 diffuse_texture;
    u16 model;
} MATERIAL_SHADER_UNIFORM_LOCATIONS;

typedef struct UI_SHADER_UNIFORM_LOCATIONS {
    u16 projection;
    u16 view;
    u16 diffuse_colour;
    u16 diffuse_texture;
    u16 model;
} UI_SHADER_UNIFORM_LOCATIONS;

typedef struct MATERIAL_SYSTEM_STATE {
    MATERIAL_SYSTEM_CONFIG config;

    MATERIAL default_material;

    // Array of registered materials.
    MATERIAL* registered_materials;

    // Hashtable for material lookups.
    HASHTABLE registered_material_table;

    // Known locations for the material shader.
    MATERIAL_SHADER_UNIFORM_LOCATIONS material_locations;
    u32 material_shader_id;

    // Known locations for the UI shader.
    UI_SHADER_UNIFORM_LOCATIONS ui_locations;
    u32 ui_shader_id;
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

    state_ptr->material_shader_id = INVALID_ID;
    state_ptr->material_locations.diffuse_colour = INVALID_ID_U16;
    state_ptr->material_locations.diffuse_texture = INVALID_ID_U16;

    state_ptr->ui_shader_id = INVALID_ID;
    state_ptr->ui_locations.diffuse_colour = INVALID_ID_U16;
    state_ptr->ui_locations.diffuse_texture = INVALID_ID_U16;

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
    // Load material configuration from resource;
    RESOURCE material_resource;
    if (!resource_system_load(name, RESOURCE_TYPE_MATERIAL, &material_resource)) {
        PRINT_ERROR("Failed to load material resource, returning nullptr.");
        return 0;
    }

    // Now acquire from loaded config.
    MATERIAL* m = NULL;
    if (material_resource.data) {
        m = material_system_acquire_from_config(*(MATERIAL_CONFIG*)material_resource.data);
    }

    // Clean up
    resource_system_unload(&material_resource);

    if (!m) {
        PRINT_ERROR("Failed to load material resource, returning nullptr.");
    }

    return m;
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

            // Get the uniform indices.
            SHADER* s = shader_system_get_by_id(m->shader_id);
            // Save off the locations for known types for quick lookups.
            PRINT_DEBUG("Material shader id: %d, name: '%s'.", s->id, s->name);
            if (state_ptr->material_shader_id == INVALID_ID && strings_equal(config.shader_name, BUILTIN_SHADER_NAME_MATERIAL)) {
                state_ptr->material_shader_id = s->id;
                PRINT_DEBUG("Material shader id set to %d for material '%s'.", state_ptr->material_shader_id, config.name);
                state_ptr->material_locations.projection = shader_system_uniform_index(s, "projection");
                state_ptr->material_locations.view = shader_system_uniform_index(s, "view");
                state_ptr->material_locations.ambient_colour = shader_system_uniform_index(s, "ambient_colour");
                state_ptr->material_locations.diffuse_colour = shader_system_uniform_index(s, "diffuse_colour");
                state_ptr->material_locations.diffuse_texture = shader_system_uniform_index(s, "diffuse_texture");
                state_ptr->material_locations.model = shader_system_uniform_index(s, "model");
            } else if (state_ptr->ui_shader_id == INVALID_ID && strings_equal(config.shader_name, BUILTIN_SHADER_NAME_UI)) {
                state_ptr->ui_shader_id = s->id;
                PRINT_DEBUG("UI shader id set to %d for material '%s'.", state_ptr->ui_shader_id, config.name);
                state_ptr->ui_locations.projection = shader_system_uniform_index(s, "projection");
                state_ptr->ui_locations.view = shader_system_uniform_index(s, "view");
                state_ptr->ui_locations.diffuse_colour = shader_system_uniform_index(s, "diffuse_colour");
                state_ptr->ui_locations.diffuse_texture = shader_system_uniform_index(s, "diffuse_texture");
                state_ptr->ui_locations.model = shader_system_uniform_index(s, "model");
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

#define MATERIAL_APPLY_OR_FAIL(expr)                  \
    if (!expr) {                                      \
        PRINT_ERROR("Failed to apply material: %s", expr); \
        return false;                                 \
    }

b8 material_system_apply_global(u32 shader_id, const Matrice4* projection, const Matrice4* view, const Vector4* ambient_colour) {
    if (shader_id == state_ptr->material_shader_id) {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.view, view));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.ambient_colour, ambient_colour));
    } else if (shader_id == state_ptr->ui_shader_id) {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.view, view));
    } else {
        PRINT_ERROR("material_system_apply_global(): Unrecognized shader id '%d' ", shader_id);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(shader_system_apply_global());
    return true;
}

b8 material_system_apply_instance(MATERIAL* m) {
    // Apply instance-level uniforms.
    MATERIAL_APPLY_OR_FAIL(shader_system_bind_instance(m->internal_id));
    if (m->shader_id == state_ptr->material_shader_id) {
        // Material shader
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.diffuse_colour, &m->diffuse_colour));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.diffuse_texture, m->diffuse_map.texture));
    } else if (m->shader_id == state_ptr->ui_shader_id) {
        // UI shader
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.diffuse_colour, &m->diffuse_colour));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.diffuse_texture, m->diffuse_map.texture));
    } else {
        PRINT_ERROR("material_system_apply_instance(): Unrecognized shader id '%d' on shader '%s'.", m->shader_id, m->name);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(shader_system_apply_instance());

    return true;
}

b8 material_system_apply_local(MATERIAL* m, const Matrice4* model) {
    if (m->shader_id == state_ptr->material_shader_id) {
        return shader_system_uniform_set_by_index(state_ptr->material_locations.model, model);
    } else if (m->shader_id == state_ptr->ui_shader_id) {
        return shader_system_uniform_set_by_index(state_ptr->ui_locations.model, model);
    }

    PRINT_ERROR("Unrecognized shader id '%d'", m->shader_id);
    return false;
}

b8 load_material(MATERIAL_CONFIG config, MATERIAL* m) {
    yzero_memory(m, sizeof(MATERIAL));

    // name
    string_ncopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    m->shader_id = shader_system_get_id(config.shader_name);

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
    SHADER* s = shader_system_get(config.shader_name);
    if (!s) {
        PRINT_ERROR("Unable to load material because its shader was not found: '%s'. This is likely a problem with the material asset.", config.shader_name);
        return false;
    }

    if (!renderer_shader_acquire_instance_resources(s, &m->internal_id)) {
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
    if (m->shader_id != INVALID_ID && m->internal_id != INVALID_ID) {
        renderer_shader_release_instance_resources(shader_system_get_by_id(m->shader_id), m->internal_id);
        m->shader_id = INVALID_ID;
    }

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

    SHADER* s = shader_system_get(BUILTIN_SHADER_NAME_MATERIAL);
    if (!renderer_shader_acquire_instance_resources(s, &state->default_material.internal_id)) {
        PRINT_ERROR("Failed to acquire renderer resources for default texture. Application cannot continue.");
        return false;
    }

    // Make sure to assign the shader id.Add commentMore actions
    state->default_material.shader_id = s->id;

    return true;
}
