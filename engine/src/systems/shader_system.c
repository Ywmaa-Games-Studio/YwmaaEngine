#include "shader_system.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"

#include "variants/darray.h"
#include "renderer/renderer_frontend.h"

#include "systems/texture_system.h"

// The internal shader system state.
typedef struct SHADER_SYSTEM_STATE {
    // This system's configuration.
    SHADER_SYSTEM_CONFIG config;
    // A lookup table for shader name->id
    HASHTABLE lookup;
    // The memory used for the lookup table.
    void* lookup_memory;
    // The identifier for the currently bound shader.
    u32 current_shader_id;
    // A collection of created shaders.
    SHADER* shaders;
} SHADER_SYSTEM_STATE;

// A pointer to hold the internal system state.
static SHADER_SYSTEM_STATE* state_ptr = 0;

b8 add_attribute(SHADER* shader, const SHADER_ATTRIBUTE_CONFIG* config);
b8 add_sampler(SHADER* shader, SHADER_UNIFORM_CONFIG* config);
b8 add_uniform(SHADER* shader, SHADER_UNIFORM_CONFIG* config);
u32 get_shader_id(const char* shader_name);
u32 new_shader_id(void);
b8 uniform_add(SHADER* shader, const char* uniform_name, u32 size, E_SHADER_UNIFORM_TYPE type, SHADER_SCOPE scope, u32 set_location, b8 is_sampler);
b8 uniform_name_valid(SHADER* shader, const char* uniform_name);
b8 shader_uniform_add_state_valid(SHADER* shader);
void shader_destroy(SHADER* s);
///////////////////////

b8 shader_system_init(u64* memory_requirement, void* memory, SHADER_SYSTEM_CONFIG config) {
    // Verify configuration.
    if (config.max_shader_count < 512) {
        if (config.max_shader_count == 0) {
            PRINT_ERROR("shader_system_init - config.max_shader_count must be greater than 0");
            return false;
        } else {
            // This is to help avoid hashtable collisions.
            PRINT_WARNING("shader_system_init - config.max_shader_count is recommended to be at least 512.");
        }
    }

    // Figure out how large of a hashtable is needed.
    // Block of memory will contain state structure then the block for the hashtable.
    u64 struct_requirement = sizeof(SHADER_SYSTEM_STATE);
    u64 hashtable_requirement = sizeof(u32) * config.max_shader_count;
    u64 shader_array_requirement = sizeof(SHADER) * config.max_shader_count;
    *memory_requirement = struct_requirement + hashtable_requirement + shader_array_requirement;

    if (!memory) {
        return true;
    }

    // Setup the state pointer, memory block, shader array, then create the hashtable.
    state_ptr = memory;
    u64 addr = (u64)memory;
    state_ptr->lookup_memory = (void*)(addr + struct_requirement);
    state_ptr->shaders = (void*)((u64)state_ptr->lookup_memory + hashtable_requirement);
    state_ptr->config = config;
    state_ptr->current_shader_id = INVALID_ID;
    hashtable_create(sizeof(u32), config.max_shader_count, state_ptr->lookup_memory, false, &state_ptr->lookup);

    // Invalidate all shader ids.
    for (u32 i = 0; i < config.max_shader_count; ++i) {
        state_ptr->shaders[i].id = INVALID_ID;
    }

    // Fill the table with invalid ids.
    u32 invalid_fill_id = INVALID_ID;
    if (!hashtable_fill(&state_ptr->lookup, &invalid_fill_id)) {
        PRINT_ERROR("hashtable_fill failed.");
        return false;
    }

    for (u32 i = 0; i < state_ptr->config.max_shader_count; ++i) {
        state_ptr->shaders[i].id = INVALID_ID;
    }

    return true;
}

void shader_system_shutdown(void* state) {
    if (state) {
        // Destroy any shaders still in existence.
        SHADER_SYSTEM_STATE* st = (SHADER_SYSTEM_STATE*)state;
        for (u32 i = 0; i < st->config.max_shader_count; ++i) {
            SHADER* s = &st->shaders[i];
            if (s->id != INVALID_ID) {
                shader_destroy(s);
            }
        }
        hashtable_destroy(&st->lookup);
        yzero_memory(st, sizeof(SHADER_SYSTEM_STATE));
    }

    state_ptr = 0;
}

b8 shader_system_create(const SHADER_CONFIG* config, E_RENDERER_BACKEND_API backend_api) {
    u32 id = new_shader_id();
    SHADER* out_shader = &state_ptr->shaders[id];
    yzero_memory(out_shader, sizeof(SHADER));
    out_shader->id = id;
    if (out_shader->id == INVALID_ID) {
        PRINT_ERROR("Unable to find free slot to create new shader. Aborting.");
        return false;
    }
    out_shader->state = SHADER_STATE_NOT_CREATED;
    out_shader->name = string_duplicate(config->name);
    out_shader->use_instances = config->use_instances;
    out_shader->use_locals = config->use_local;
    out_shader->push_constant_range_count = 0;
    yzero_memory(out_shader->push_constant_ranges, sizeof(range) * 32);
    out_shader->bound_instance_id = INVALID_ID;
    out_shader->attribute_stride = 0;

    // Setup arrays
    out_shader->global_textures = darray_create(TEXTURE*);
    out_shader->uniforms = darray_create(SHADER_UNIFORM);
    out_shader->attributes = darray_create(SHADER_ATTRIBUTE);

    // Create a hashtable to store uniform array indexes. This provides a direct index into the
    // 'uniforms' array stored in the shader for quick lookups by name.
    u64 element_size = sizeof(u16);  // Indexes are stored as u16s.
    u64 element_count = 1024;        // This is more uniforms than we will ever need, but a bigger table reduces collision chance.
    out_shader->hashtable_block = yallocate(element_size * element_count, MEMORY_TAG_DICT);
    hashtable_create(element_size, element_count, out_shader->hashtable_block, false, &out_shader->uniform_lookup);

    // Invalidate all spots in the hashtable.
    u32 invalid = INVALID_ID;
    hashtable_fill(&out_shader->uniform_lookup, &invalid);

    // A running total of the actual global uniform buffer object size.
    out_shader->global_ubo_size = 0;
    // A running total of the actual instance uniform buffer object size.
    out_shader->ubo_size = 0;
    // NOTE: UBO alignment requirement set in renderer backend.

    // This is hard-coded because the Vulkan spec only guarantees that a _minimum_ 128 bytes of space are available,
    // and it's up to the driver to determine how much is available. Therefore, to avoid complexity, only the
    // lowest common denominator of 128B will be used.
    out_shader->push_constant_stride = 128;
    out_shader->push_constant_size = 0;

    u8 renderpass_id = INVALID_ID_U8;
    if (!renderer_renderpass_id(config->renderpass_name, &renderpass_id)) {
        PRINT_ERROR("Unable to find renderpass '%s'", config->renderpass_name);
        return false;
    }

    switch (backend_api)
    {
        case RENDERER_BACKEND_API_VULKAN:
            // Create the shader with the renderer.
            if (!renderer_shader_create(out_shader, renderpass_id, config->stage_count, (const char**)config->vulkan_stage_filenames, config->stages)) {
                PRINT_ERROR("Error creating shader.");
                return false;
            }
            break;
        case RENDERER_BACKEND_API_WEBGPU:
            // Create the shader with the renderer.
            if (!renderer_shader_create(out_shader, renderpass_id, config->stage_count, (const char**)config->webgpu_stage_filenames, config->stages)) {
                PRINT_ERROR("Error creating shader.");
                return false;
            }
            break;
        default:
            break;
    }



    // Ready to be initialized.
    out_shader->state = SHADER_STATE_UNINITIALIZED;

    // Process attributes
    for (u32 i = 0; i < config->attribute_count; ++i) {
        add_attribute(out_shader, &config->attributes[i]);
    }

    // Process uniforms
    for (u32 i = 0; i < config->uniform_count; ++i) {
        if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER) {
            add_sampler(out_shader, &config->uniforms[i]);
        } else {
            add_uniform(out_shader, &config->uniforms[i]);
        }
    }

    // Initialize the shader.
    if (!renderer_shader_init(out_shader)) {
        PRINT_ERROR("shader_system_create: initialization failed for shader '%s'.", config->name);
        // NOTE: initialize automatically destroys the shader if it fails.
        return false;
    }

    // At this point, creation is successful, so store the shader id in the hashtable
    // so this can be looked up by name later.
    if (!hashtable_set(&state_ptr->lookup, config->name, &out_shader->id)) {
        // Dangit, we got so far... welp, nuke the shader and boot.
        renderer_shader_destroy(out_shader);
        return false;
    }

    return true;
}

u32 shader_system_get_id(const char* shader_name) {
    return get_shader_id(shader_name);
}

SHADER* shader_system_get_by_id(u32 shader_id) {
    if (shader_id >= state_ptr->config.max_shader_count || state_ptr->shaders[shader_id].id == INVALID_ID) {
        return 0;
    }
    return &state_ptr->shaders[shader_id];
}

SHADER* shader_system_get(const char* shader_name) {
    u32 shader_id = get_shader_id(shader_name);
    if (shader_id != INVALID_ID) {
        return shader_system_get_by_id(shader_id);
    }
    return 0;
}

void shader_destroy(SHADER* s) {
    renderer_shader_destroy(s);

    // Set it to be unusable right away.
    s->state = SHADER_STATE_NOT_CREATED;

    // Free the name.
    if (s->name) {
        yfree(s->name, MEMORY_TAG_STRING);
    }
    s->name = 0;
}

void shader_system_destroy(const char* shader_name) {
    u32 shader_id = get_shader_id(shader_name);
    if (shader_id == INVALID_ID) {
        return;
    }

    SHADER* s = &state_ptr->shaders[shader_id];

    shader_destroy(s);
}

b8 shader_system_use(const char* shader_name) {
    u32 next_shader_id = get_shader_id(shader_name);
    if (next_shader_id == INVALID_ID) {
        return false;
    }

    return shader_system_use_by_id(next_shader_id);
}

b8 shader_system_use_by_id(u32 shader_id) {
    // Only perform the use if the shader id is different.
    if (state_ptr->current_shader_id != shader_id) {
        SHADER* next_shader = shader_system_get_by_id(shader_id);
        state_ptr->current_shader_id = shader_id;
        if (!renderer_shader_use(next_shader)) {
            PRINT_ERROR("Failed to use shader '%s'.", next_shader->name);
            return false;
        }
        if (!renderer_shader_bind_globals(next_shader)) {
            PRINT_ERROR("Failed to bind globals for shader '%s'.", next_shader->name);
            return false;
        }
    }
    return true;
}

b8 shader_system_after_renderpass(u32 shader_id){
    // Only perform the use if the shader id is different.
    SHADER* shader = shader_system_get_by_id(shader_id);
    state_ptr->current_shader_id = shader_id;
    if (!renderer_shader_after_renderpass(shader)) {
        PRINT_ERROR("Failed to shader_after_renderpass for shader '%s'.", shader->name);
        return false;
    }
    return true;
}

u16 shader_system_uniform_index(SHADER* s, const char* uniform_name) {
    if (!s || s->id == INVALID_ID) {
        PRINT_ERROR("shader_system_uniform_location called with invalid shader.");
        return INVALID_ID_U16;
    }

    u16 index = INVALID_ID_U16;
    if (!hashtable_get(&s->uniform_lookup, uniform_name, &index) || index == INVALID_ID_U16) {
        PRINT_ERROR("Shader '%s' does not have a registered uniform named '%s'", s->name, uniform_name);
        return INVALID_ID_U16;
    }
    return s->uniforms[index].index;
}

b8 shader_system_uniform_set(const char* uniform_name, const void* value) {
    if (state_ptr->current_shader_id == INVALID_ID) {
        PRINT_ERROR("shader_system_uniform_set called without a shader in use.");
        return false;
    }
    SHADER* s = &state_ptr->shaders[state_ptr->current_shader_id];
    u16 index = shader_system_uniform_index(s, uniform_name);
    return shader_system_uniform_set_by_index(index, value);
}

b8 shader_system_sampler_set(const char* sampler_name, const TEXTURE* t) {
    return shader_system_uniform_set(sampler_name, t);
}

b8 shader_system_uniform_set_by_index(u16 index, const void* value) {
    SHADER* shader = &state_ptr->shaders[state_ptr->current_shader_id];
    SHADER_UNIFORM* uniform = &shader->uniforms[index];
    if (shader->bound_scope != uniform->scope) {
        if (uniform->scope == SHADER_SCOPE_GLOBAL) {
            renderer_shader_bind_globals(shader);
        } else if (uniform->scope == SHADER_SCOPE_INSTANCE) {
            renderer_shader_bind_instance(shader, shader->bound_instance_id);
        } else {
            // NOTE: Nothing to do here for locals, just set the uniform.
        }
        shader->bound_scope = uniform->scope;
    }
    return renderer_set_uniform(shader, uniform, value);
}
b8 shader_system_sampler_set_by_index(u16 index, const TEXTURE* t) {
    return shader_system_uniform_set_by_index(index, t);
}

b8 shader_system_apply_global(void) {
    return renderer_shader_apply_globals(&state_ptr->shaders[state_ptr->current_shader_id]);
}
b8 shader_system_apply_instance(b8 needs_update) {
    return renderer_shader_apply_instance(&state_ptr->shaders[state_ptr->current_shader_id], needs_update);
}

b8 shader_system_bind_instance(u32 instance_id) {
    SHADER* s = &state_ptr->shaders[state_ptr->current_shader_id];
    s->bound_instance_id = instance_id;
    return renderer_shader_bind_instance(s, instance_id);
}

b8 add_attribute(SHADER* shader, const SHADER_ATTRIBUTE_CONFIG* config) {
    u32 size = 0;
    switch (config->type) {
        case SHADER_ATTRIB_TYPE_INT8:
        case SHADER_ATTRIB_TYPE_UINT8:
            size = 1;
            break;
        case SHADER_ATTRIB_TYPE_INT16:
        case SHADER_ATTRIB_TYPE_UINT16:
            size = 2;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32:
        case SHADER_ATTRIB_TYPE_INT32:
        case SHADER_ATTRIB_TYPE_UINT32:
            size = 4;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_2:
            size = 8;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_3:
            size = 12;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_4:
            size = 16;
            break;
        default:
            PRINT_ERROR("Unrecognized type %d, defaulting to size of 4. This probably is not what is desired.");
            size = 4;
            break;
    }

    shader->attribute_stride += size;

    // Create/push the attribute.
    SHADER_ATTRIBUTE attrib = {0};
    attrib.name = string_duplicate(config->name);
    attrib.size = size;
    attrib.type = config->type;
    darray_push(shader->attributes, attrib);

    return true;
}

b8 add_sampler(SHADER* shader, SHADER_UNIFORM_CONFIG* config) {
    if (config->scope == SHADER_SCOPE_INSTANCE && !shader->use_instances) {
        PRINT_ERROR("add_sampler cannot add an instance sampler for a shader that does not use instances.");
        return false;
    }

    // Samples can't be used for push constants.
    if (config->scope == SHADER_SCOPE_LOCAL) {
        PRINT_ERROR("add_sampler cannot add a sampler at local scope.");
        return false;
    }

    // Verify the name is valid and unique.
    if (!uniform_name_valid(shader, config->name) || !shader_uniform_add_state_valid(shader)) {
        return false;
    }

    // If global, push into the global list.
    u32 location = 0;
    if (config->scope == SHADER_SCOPE_GLOBAL) {
        u32 global_texture_count = darray_length(shader->global_textures);
        if (global_texture_count + 1 > state_ptr->config.max_global_textures) {
            PRINT_ERROR("Shader global texture count %i exceeds max of %i", global_texture_count, state_ptr->config.max_global_textures);
            return false;
        }
        location = global_texture_count;
        darray_push(shader->global_textures, texture_system_get_default_texture());
    } else {
        // Otherwise, it's instance-level, so keep count of how many need to be added during the resource acquisition.
        if (shader->instance_texture_count + 1 > state_ptr->config.max_instance_textures) {
            PRINT_ERROR("Shader instance texture count %i exceeds max of %i", shader->instance_texture_count, state_ptr->config.max_instance_textures);
            return false;
        }
        location = shader->instance_texture_count;
        shader->instance_texture_count++;
    }

    // Treat it like a uniform. NOTE: In the case of samplers, out_location is used to determine the
    // hashtable entry's 'location' field value directly, and is then set to the index of the uniform array.
    // This allows location lookups for samplers as if they were uniforms as well (since technically they are).
    // TODO: might need to store this elsewhere
    if (!uniform_add(shader, config->name, 0, config->type, config->scope, location, true)) {
        PRINT_ERROR("Unable to add sampler uniform.");
        return false;
    }

    return true;
}

b8 add_uniform(SHADER* shader, SHADER_UNIFORM_CONFIG* config) {
    if (!shader_uniform_add_state_valid(shader) || !uniform_name_valid(shader, config->name)) {
        return false;
    }
    return uniform_add(shader, config->name, config->size, config->type, config->scope, 0, false);
}

u32 get_shader_id(const char* shader_name) {
    u32 shader_id = INVALID_ID;
    if (!hashtable_get(&state_ptr->lookup, shader_name, &shader_id)) {
        PRINT_ERROR("There is no shader registered named '%s'.", shader_name);
        return INVALID_ID;
    }
    return shader_id;
}

u32 new_shader_id(void) {
    for (u32 i = 0; i < state_ptr->config.max_shader_count; ++i) {
        if (state_ptr->shaders[i].id == INVALID_ID) {
            return i;
        }
    }
    return INVALID_ID;
}

b8 uniform_add(SHADER* shader, const char* uniform_name, u32 size, E_SHADER_UNIFORM_TYPE type, SHADER_SCOPE scope, u32 set_location, b8 is_sampler) {
    u32 uniform_count = darray_length(shader->uniforms);
    if (uniform_count + 1 > state_ptr->config.max_uniform_count) {
        PRINT_ERROR("A shader can only accept a combined maximum of %d uniforms and samplers at global, instance and local scopes.", state_ptr->config.max_uniform_count);
        return false;
    }
    SHADER_UNIFORM entry;
    entry.index = uniform_count;  // Index is saved to the hashtable for lookups.
    entry.scope = scope;
    entry.type = type;
    b8 is_global = (scope == SHADER_SCOPE_GLOBAL);
    if (is_sampler) {
        // Just use the passed in location
        entry.location = set_location;
    } else {
        entry.location = entry.index;
    }

    if (scope != SHADER_SCOPE_LOCAL) {
        entry.set_index = (u32)scope;
        entry.offset = is_sampler ? 0 : is_global ? shader->global_ubo_size
                                                  : shader->ubo_size;
        entry.size = is_sampler ? 0 : size;
    } else {
        if (entry.scope == SHADER_SCOPE_LOCAL && !shader->use_locals) {
            PRINT_ERROR("Cannot add a locally-scoped uniform for a shader that does not support locals.");
            return false;
        }
        // Push a new aligned range (align to 4, as required by Vulkan spec)
        entry.set_index = INVALID_ID_U8;
        range r = get_aligned_range(shader->push_constant_size, size, 4);
        // utilize the aligned offset/range
        entry.offset = r.offset;
        entry.size = r.size;

        // Track in configuration for use in initialization.
        shader->push_constant_ranges[shader->push_constant_range_count] = r;
        shader->push_constant_range_count++;

        // Increase the push constant's size by the total value.
        shader->push_constant_size += r.size;
    }

    if (!hashtable_set(&shader->uniform_lookup, uniform_name, &entry.index)) {
        PRINT_ERROR("Failed to add uniform.");
        return false;
    }
    darray_push(shader->uniforms, entry);

    if (!is_sampler) {
        if (entry.scope == SHADER_SCOPE_GLOBAL) {
            shader->global_ubo_size += entry.size;
        } else if (entry.scope == SHADER_SCOPE_INSTANCE) {
            shader->ubo_size += entry.size;
        }
    }

    return true;
}

b8 uniform_name_valid(SHADER* shader, const char* uniform_name) {
    if (!uniform_name || !string_length(uniform_name)) {
        PRINT_ERROR("Uniform name must exist.");
        return false;
    }
    u16 location;
    if (hashtable_get(&shader->uniform_lookup, uniform_name, &location) && location != INVALID_ID_U16) {
        PRINT_ERROR("A uniform by the name '%s' already exists on shader '%s'.", uniform_name, shader->name);
        return false;
    }
    return true;
}

b8 shader_uniform_add_state_valid(SHADER* shader) {
    if (shader->state != SHADER_STATE_UNINITIALIZED) {
        PRINT_ERROR("Uniforms may only be added to shaders before initialization.");
        return false;
    }
    return true;
}
