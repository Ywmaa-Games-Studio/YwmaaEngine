#include "mesh.h"

#include "core/ymemory.h"
#include "core/logger.h"
#include "core/identifier.h"
#include "core/ystring.h"
#include "systems/job_system.h"

#include "systems/resource_system.h"
#include "systems/geometry_system.h"
#include "renderer/renderer_types.inl"

// Also used as result_data from job.
typedef struct MESH_LOAD_PARAMS {
    const char* resource_name;
    Mesh* out_mesh;
    RESOURCE mesh_resource;
} MESH_LOAD_PARAMS;

/**
 * @brief Called when the job completes successfully.
 *
 * @param params The parameters passed from the job after completion.
 */
void mesh_load_job_success(void* params) {
    MESH_LOAD_PARAMS* mesh_params = (MESH_LOAD_PARAMS*)params;

    // This also handles the GPU upload. Can't be jobified until the renderer is multithreaded.
    GEOMETRY_CONFIG* configs = (GEOMETRY_CONFIG*)mesh_params->mesh_resource.data;
    mesh_params->out_mesh->geometry_count = mesh_params->mesh_resource.data_size;
    mesh_params->out_mesh->geometries = yallocate_aligned(sizeof(GEOMETRY*) * mesh_params->out_mesh->geometry_count, 8, MEMORY_TAG_ARRAY);
    for (u32 i = 0; i < mesh_params->out_mesh->geometry_count; ++i) {
        mesh_params->out_mesh->geometries[i] = geometry_system_acquire_from_config(configs[i], true);
    }
    mesh_params->out_mesh->generation++;

    PRINT_TRACE("Successfully loaded mesh '%s'.", mesh_params->resource_name);

    resource_system_unload(&mesh_params->mesh_resource);
}

/**
 * @brief Called when the job fails.
 *
 * @param params Parameters passed when a job fails.
 */
void mesh_load_job_fail(void* params) {
    MESH_LOAD_PARAMS* mesh_params = (MESH_LOAD_PARAMS*)params;

    PRINT_ERROR("Failed to load mesh '%s'.", mesh_params->resource_name);

    resource_system_unload(&mesh_params->mesh_resource);
}

/**
 * @brief Called when a mesh loading job begins.
 *
 * @param params Mesh loading parameters.
 * @param result_data Result data passed to the completion callback.
 * @return True on job success; otherwise false.
 */
b8 mesh_load_job_start(void* params, void* result_data) {
    MESH_LOAD_PARAMS* load_params = (MESH_LOAD_PARAMS*)params;
    b8 result = resource_system_load(load_params->resource_name, RESOURCE_TYPE_MESH, 0, &load_params->mesh_resource);

    // NOTE: The load params are also used as the result data here, only the mesh_resource field is populated now.
    ycopy_memory(result_data, load_params, sizeof(MESH_LOAD_PARAMS));

    return result;
}

static b8 mesh_load_from_resource(const char* resource_name, Mesh* out_mesh) {
    out_mesh->generation = INVALID_ID_U8;

    MESH_LOAD_PARAMS params;
    params.resource_name = resource_name;
    params.out_mesh = out_mesh;
    params.mesh_resource = (RESOURCE){0};

    JOB_INFO job = job_create(mesh_load_job_start, mesh_load_job_success, mesh_load_job_fail, &params, sizeof(MESH_LOAD_PARAMS), sizeof(MESH_LOAD_PARAMS));
    job_system_submit(job);

    return true;
}

b8 mesh_create(MESH_CONFIG config, Mesh* out_mesh) {
    if (!out_mesh) {
        return false;
    }

    yzero_memory(out_mesh, sizeof(Mesh));

    out_mesh->config = config;
    out_mesh->generation = INVALID_ID_U8;

    if (config.name) {
        out_mesh->name = string_duplicate(config.name);
    }

    return true;
}

b8 mesh_init(Mesh* m) {
    if (!m) {
        return false;
    }

    if (m->config.resource_name) {
        return true;
    } else {
        // Just verifying config.
        if (!m->config.g_configs) {
            return false;
        }

        m->geometry_count = m->config.geometry_count;
        m->geometries = yallocate(sizeof(GEOMETRY*), MEMORY_TAG_ARRAY);
    }
    return true;
}

b8 mesh_load(Mesh* m) {
    if (!m) {
        return false;
    }

    m->unique_id = identifier_aquire_new_id(m);

    if (m->config.resource_name) {
        return mesh_load_from_resource(m->config.resource_name, m);
    } else {
        if (!m->config.g_configs) {
            return false;
        }

        for (u32 i = 0; i < m->config.geometry_count; ++i) {
            m->geometries[i] = geometry_system_acquire_from_config(m->config.g_configs[i], true);
            m->generation = 0;

            // Clean up the allocations for the geometry config.
            // TODO: Do this during unload/destroy
            geometry_system_config_dispose(&m->config.g_configs[i]);
        }
    }

    return true;
}

b8 mesh_unload(Mesh* m) {
    if (m) {
        for (u32 i = 0; i < m->geometry_count; ++i) {
            geometry_system_release(m->geometries[i]);
        }

        yfree(m->geometries);
        yzero_memory(m, sizeof(Mesh));

        // For good measure, invalidate the geometry so it doesn't attempt to be rendered.
        m->generation = INVALID_ID_U8;
        return true;
    }
    return false;
}

b8 mesh_destroy(Mesh* m) {
    if (!m) {
        return false;
    }

    if (m->geometries) {
        if (!mesh_unload(m)) {
            PRINT_ERROR("mesh_destroy - failed to unload mesh.");
            return false;
        }
    }

    if (m->name) {
        yfree(m->name);
        m->name = 0;
    }

    if (m->config.name) {
        yfree(m->config.name);
        m->config.name = 0;
    }
    if (m->config.resource_name) {
        yfree(m->config.resource_name);
        m->config.resource_name = 0;
    }
    if (m->config.parent_name) {
        yfree(m->config.parent_name);
        m->config.parent_name = 0;
    }

    return true;
}
