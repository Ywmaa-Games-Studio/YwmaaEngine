#include "mesh.h"

#include "core/ymemory.h"
#include "core/logger.h"
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
    mesh_params->out_mesh->geometries = yallocate(sizeof(GEOMETRY*) * mesh_params->out_mesh->geometry_count, MEMORY_TAG_ARRAY);
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

b8 mesh_load_from_resource(const char* resource_name, Mesh* out_mesh) {
    out_mesh->generation = INVALID_ID_U8;

    MESH_LOAD_PARAMS params;
    params.resource_name = resource_name;
    params.out_mesh = out_mesh;
    params.mesh_resource = (RESOURCE){0};

    JOB_INFO job = job_create(mesh_load_job_start, mesh_load_job_success, mesh_load_job_fail, &params, sizeof(MESH_LOAD_PARAMS), sizeof(MESH_LOAD_PARAMS));
    job_system_submit(job);

    return true;
}

void mesh_unload(Mesh* m) {
    if (m) {
        for (u32 i = 0; i < m->geometry_count; ++i) {
            geometry_system_release(m->geometries[i]);
        }

        yfree(m->geometries);
        yzero_memory(m, sizeof(Mesh));

        // For good measure, invalidate the geometry so it doesn't attempt to be rendered.
        m->generation = INVALID_ID_U8;
    }
}
