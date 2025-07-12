#include "mesh_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "data_structures/darray.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"
#include "math/ymath.h"
#include "math/geometry_utils.h"
#include "loader_utils.h"

#include "io/filesystem.h"

#include <stdio.h>  //sscanf
#define CGLTF_IMPLEMENTATION
#include "vendor/cgltf.h"

typedef enum E_MESH_FILE_TYPE {
    MESH_FILE_TYPE_NOT_FOUND,
    MESH_FILE_TYPE_YMESH,
    MESH_FILE_TYPE_OBJ,
    MESH_FILE_TYPE_GLB,
    MESH_FILE_TYPE_GLTF,
} E_MESH_FILE_TYPE;

typedef struct SUPPORTED_MESH_FILETYPE {
    char* extension;
    E_MESH_FILE_TYPE type;
    b8 is_binary;
} SUPPORTED_MESH_FILETYPE;

typedef struct MESH_VERTEX_INDEX_DATA {
    u32 position_index;
    u32 normal_index;
    u32 texcoord_index;
} MESH_VERTEX_INDEX_DATA;

typedef struct MESH_FACE_DATA {
    MESH_VERTEX_INDEX_DATA vertices[3];
} MESH_FACE_DATA;

typedef struct MESH_GROUP_DATA {
    // darray
    MESH_FACE_DATA* faces;
} MESH_GROUP_DATA;

//gltf error messages
const char* cgltf_error_messages[] = {
    "No error",
    "Invalid JSON",
    "Invalid file format",
    "Invalid file version",
    "Invalid file structure",
    "Invalid data type",
    "Invalid data value",
    "Out of memory",
    "Unknown error"
};

b8 import_gltf_file(FILE_HANDLE* gltf_file, const char* path, const char* out_y_mesh_filename, GEOMETRY_CONFIG** out_geometries_darray, b8 is_binary);
b8 import_obj_file(FILE_HANDLE* obj_file, const char* out_y_mesh_filename, GEOMETRY_CONFIG** out_geometries_darray);
void process_subobject(Vector3* positions, Vector3* normals, Vector2* tex_coords, MESH_FACE_DATA* faces, GEOMETRY_CONFIG* out_data);
b8 import_obj_material_library_file(const char* mtl_file_path);
b8 extract_gltf_texture(const cgltf_image* image, const char* base_path, char* out_filename);

b8 load_y_mesh_file(FILE_HANDLE* y_mesh_file, GEOMETRY_CONFIG** out_geometries_darray);
b8 write_y_mesh_file(const char* path, const char* name, u32 geometry_count, GEOMETRY_CONFIG* geometries);
b8 write_y_material_file(const char* directory, MATERIAL_CONFIG* config);

b8 mesh_loader_load(struct RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    FILE_HANDLE f;
    // Supported extensions. Note that these are in order of priority when looked up.
    // This is to prioritize the loading of a binary version of the mesh, followed by
    // importing various types of meshes to binary types, which would be loaded on the
    // next run.
    // TODO: Might be good to be able to specify an override to always import (i.e. skip
    // binary versions) for debug purposes.
#define SUPPORTED_FILETYPE_COUNT 4
    SUPPORTED_MESH_FILETYPE supported_filetypes[SUPPORTED_FILETYPE_COUNT];
    supported_filetypes[0] = (SUPPORTED_MESH_FILETYPE){".y_mesh", MESH_FILE_TYPE_YMESH, true};
    supported_filetypes[1] = (SUPPORTED_MESH_FILETYPE){".obj", MESH_FILE_TYPE_OBJ, false};
    supported_filetypes[2] = (SUPPORTED_MESH_FILETYPE){".glb", MESH_FILE_TYPE_GLB, true};
    supported_filetypes[3] = (SUPPORTED_MESH_FILETYPE){".gltf", MESH_FILE_TYPE_GLTF, false};

    char full_file_path[512];
    E_MESH_FILE_TYPE type = MESH_FILE_TYPE_NOT_FOUND;
    // Try each supported extension.
    for (u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
        string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, supported_filetypes[i].extension);
        // If the file exists, open it and stop looking.
        if (filesystem_exists(full_file_path)) {
            if (filesystem_open(full_file_path, FILE_MODE_READ, supported_filetypes[i].is_binary, &f)) {
                type = supported_filetypes[i].type;
                break;
            }
        }
    }

    if (type == MESH_FILE_TYPE_NOT_FOUND) {
        PRINT_ERROR("Unable to find mesh of supported type called '%s'.", name);
        return false;
    }

    out_resource->full_path = string_duplicate(full_file_path);

    // The resource data is just an array of configs.
    GEOMETRY_CONFIG* resource_data = darray_create(GEOMETRY_CONFIG);

    b8 result = false;
    switch (type) {
        case MESH_FILE_TYPE_OBJ: {
            // Generate the y_mesh filename.
            char y_mesh_file_name[512];
            string_format(y_mesh_file_name, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".y_mesh");
            result = import_obj_file(&f, y_mesh_file_name, &resource_data);
            break;
        }
        case MESH_FILE_TYPE_GLB: {
            // Generate the y_mesh filename.
            char y_glb_file_name[512];
            string_format(y_glb_file_name, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".y_mesh");
            // gltf path
            char gltf_file_path[512];
            string_format(gltf_file_path, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".glb");
            // If the gltf file
            result = import_gltf_file(&f, gltf_file_path, y_glb_file_name, &resource_data, true);
            break;
        }
        case MESH_FILE_TYPE_GLTF: {
            // Generate the y_mesh filename.
            char y_gltf_file_name[512];
            string_format(y_gltf_file_name, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".y_mesh");
            // gltf path
            char glb_file_path[512];
            string_format(glb_file_path, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".gltf");
            result = import_gltf_file(&f, glb_file_path, y_gltf_file_name, &resource_data, false);
            break;
        }
        case MESH_FILE_TYPE_YMESH:
            result = load_y_mesh_file(&f, &resource_data);
            break;
        case MESH_FILE_TYPE_NOT_FOUND:
            PRINT_ERROR("Unable to find mesh of supported type called '%s'.", name);
            result = false;
            break;
    }

    filesystem_close(&f);

    if (!result) {
        PRINT_ERROR("Failed to process mesh file '%s'.", full_file_path);
        darray_destroy(resource_data);
        out_resource->data = 0;
        out_resource->data_size = 0;
        return false;
    }

    out_resource->data = resource_data;
    // Use the data size as a count.
    out_resource->data_size = darray_length(resource_data);

    return true;
}

void mesh_loader_unload(struct RESOURCE_LOADER* self, RESOURCE* resource) {
    u32 count = darray_length(resource->data);
    for (u32 i = 0; i < count; ++i) {
        GEOMETRY_CONFIG* config = &((GEOMETRY_CONFIG*)resource->data)[i];
        geometry_system_config_dispose(config);
    }
    darray_destroy(resource->data);
    resource->data = 0;
    resource->data_size = 0;
}

b8 load_y_mesh_file(FILE_HANDLE* y_mesh_file, GEOMETRY_CONFIG** out_geometries_darray) {
    // Version
    u64 bytes_read = 0;
    u16 version = 0;
    filesystem_read(y_mesh_file, sizeof(u16), &version, &bytes_read);

    // Name length
    u32 name_length = 0;
    filesystem_read(y_mesh_file, sizeof(u32), &name_length, &bytes_read);
    // Name + terminator
    char name[256];
    filesystem_read(y_mesh_file, sizeof(char) * name_length, name, &bytes_read);

    // Geometry count
    u32 geometry_count = 0;
    filesystem_read(y_mesh_file, sizeof(u32), &geometry_count, &bytes_read);

    // Each geometry
    for (u32 i = 0; i < geometry_count; ++i) {
        GEOMETRY_CONFIG g = {0};

        // Vertices (size/count/array)
        filesystem_read(y_mesh_file, sizeof(u32), &g.vertex_size, &bytes_read);
        filesystem_read(y_mesh_file, sizeof(u32), &g.vertex_count, &bytes_read);
        if (g.vertex_size == 0 || g.vertex_count == 0) {
            PRINT_ERROR("Invalid vertex size or count for geometry %u in mesh file '%s'.", i, name);
            return false;
        }
        g.vertices = yallocate(g.vertex_size * g.vertex_count, MEMORY_TAG_ARRAY);
        filesystem_read(y_mesh_file, g.vertex_size * g.vertex_count, g.vertices, &bytes_read);

        // Indices (size/count/array)
        filesystem_read(y_mesh_file, sizeof(u32), &g.index_size, &bytes_read);
        filesystem_read(y_mesh_file, sizeof(u32), &g.index_count, &bytes_read);
        if (g.index_size == 0 || g.index_count == 0) {
            PRINT_ERROR("Invalid index size or count for geometry %u in mesh file '%s'.", i, name);
            yfree(g.vertices);
            return false;
        }
        g.indices = yallocate(g.index_size * g.index_count, MEMORY_TAG_ARRAY);
        filesystem_read(y_mesh_file, g.index_size * g.index_count, g.indices, &bytes_read);

        // Name
        u32 g_name_length = 0;
        filesystem_read(y_mesh_file, sizeof(u32), &g_name_length, &bytes_read);
        filesystem_read(y_mesh_file, sizeof(char) * g_name_length, g.name, &bytes_read);

        // Material Name
        u32 m_name_length = 0;
        filesystem_read(y_mesh_file, sizeof(u32), &m_name_length, &bytes_read);
        filesystem_read(y_mesh_file, sizeof(char) * m_name_length, g.material_name, &bytes_read);

        // Center
        filesystem_read(y_mesh_file, sizeof(Vertex3D), &g.center, &bytes_read);

        // Extents (min/max)
        filesystem_read(y_mesh_file, sizeof(Vertex3D), &g.min_extents, &bytes_read);
        filesystem_read(y_mesh_file, sizeof(Vertex3D), &g.max_extents, &bytes_read);

        // Add to the output array.
        darray_push(*out_geometries_darray, g);
    }

    filesystem_close(y_mesh_file);

    return true;
}

b8 write_y_mesh_file(const char* path, const char* name, u32 geometry_count, GEOMETRY_CONFIG* geometries) {
    if (filesystem_exists(path)) {
        PRINT_INFO("File '%s' already exists and will be overwritten.", path);
    }

    FILE_HANDLE f;
    if (!filesystem_open(path, FILE_MODE_WRITE, true, &f)) {
        PRINT_ERROR("Unable to open file '%s' for writing. Y_Mesh write failed.", path);
        return false;
    }

    // Version
    u64 written = 0;
    u16 version = 0x0001U;
    filesystem_write(&f, sizeof(u16), &version, &written);

    // Name length
    u32 name_length = string_length(name) + 1;
    filesystem_write(&f, sizeof(u32), &name_length, &written);
    // Name + terminator
    filesystem_write(&f, sizeof(char) * name_length, name, &written);

    // Geometry count
    filesystem_write(&f, sizeof(u32), &geometry_count, &written);

    // Each geometry
    for (u32 i = 0; i < geometry_count; ++i) {
        GEOMETRY_CONFIG* g = &geometries[i];

        // Vertices (size/count/array)
        filesystem_write(&f, sizeof(u32), &g->vertex_size, &written);
        filesystem_write(&f, sizeof(u32), &g->vertex_count, &written);
        filesystem_write(&f, g->vertex_size * g->vertex_count, g->vertices, &written);

        // Indices (size/count/array)
        filesystem_write(&f, sizeof(u32), &g->index_size, &written);
        filesystem_write(&f, sizeof(u32), &g->index_count, &written);
        filesystem_write(&f, g->index_size * g->index_count, g->indices, &written);

        // Name
        u32 g_name_length = string_length(g->name) + 1;
        filesystem_write(&f, sizeof(u32), &g_name_length, &written);
        filesystem_write(&f, sizeof(char) * g_name_length, g->name, &written);

        // Material Name
        u32 m_name_length = string_length(g->material_name) + 1;
        filesystem_write(&f, sizeof(u32), &m_name_length, &written);
        filesystem_write(&f, sizeof(char) * m_name_length, g->material_name, &written);

        // Center
        filesystem_write(&f, sizeof(Vertex3D), &g->center, &written);

        // Extents (min/max)
        filesystem_write(&f, sizeof(Vertex3D), &g->min_extents, &written);
        filesystem_write(&f, sizeof(Vertex3D), &g->max_extents, &written);
    }

    filesystem_close(&f);

    return true;
}

/**
 * @brief Imports an obj file. This reads the obj, creates geometry configs, then calls logic to write
 * those geometries out to a binary y_mesh file. That file can be used on the next load.
 *
 * @param obj_file A pointer to the obj file handle to be read.
 * @param out_y_mesh_filename The path to the y_mesh file to be written to.
 * @param out_geometries_darray A darray of geometries parsed from the file.
 * @return True on success; otherwise false.
 */
b8 import_obj_file(FILE_HANDLE* obj_file, const char* out_y_mesh_filename, GEOMETRY_CONFIG** out_geometries_darray) {
    // Positions
    Vector3* positions = darray_reserve(Vector3, 16384);

    // Normals
    Vector3* normals = darray_reserve(Vector3, 16384);

    // Texture coordinates
    Vector2* tex_coords = darray_reserve(Vector2, 16384);

    // Groups
    MESH_GROUP_DATA* groups = darray_reserve(MESH_GROUP_DATA, 4);

    char material_file_name[512] = "";

    char name[512];
    u8 current_mat_name_count = 0;
    char material_names[32][64];

    char line_buf[512] = "";
    char* p = &line_buf[0];
    u64 line_length = 0;

    // index 0 is previous, 1 is previous before that.
    char prev_first_chars[2] = {0, 0};
    while (true) {
        if (!filesystem_read_line(obj_file, 511, &p, &line_length)) {
            break;
        }

        // Skip blank lines.
        if (line_length < 1) {
            continue;
        }

        char first_char = line_buf[0];

        switch (first_char) {
            case '#':
                // Skip comments
                continue;
            case 'v': {
                char second_char = line_buf[1];
                switch (second_char) {
                    case ' ': {
                        // Vertex position
                        Vector3 pos;
                        char t[2];
                        sscanf(
                            line_buf,
                            "%s %f %f %f",
                            t,
                            &pos.x,
                            &pos.y,
                            &pos.z);

                        darray_push(positions, pos);
                    } break;
                    case 'n': {
                        // Vertex normal
                        Vector3 norm;
                        char t[2];
                        sscanf(
                            line_buf,
                            "%s %f %f %f",
                            t,
                            &norm.x,
                            &norm.y,
                            &norm.z);

                        darray_push(normals, norm);
                    } break;
                    case 't': {
                        // Vertex texture coords.
                        Vector2 tex_coord;
                        char t[2];

                        // NOTE: Ignoring Z if present.
                        sscanf(
                            line_buf,
                            "%s %f %f",
                            t,
                            &tex_coord.x,
                            &tex_coord.y);

                        darray_push(tex_coords, tex_coord);
                    } break;
                }
            } break;
            case 's': {
            } break;
            case 'f': {
                // face
                // f 1/1/1 2/2/2 3/3/3  = pos/tex/norm pos/tex/norm pos/tex/norm
                MESH_FACE_DATA face;
                char t[2];

                u64 normal_count = darray_length(normals);
                u64 tex_coord_count = darray_length(tex_coords);

                if (normal_count == 0 || tex_coord_count == 0) {
                    sscanf(
                        line_buf,
                        "%s %d %d %d",
                        t,
                        &face.vertices[0].position_index,
                        &face.vertices[1].position_index,
                        &face.vertices[2].position_index);
                } else {
                    sscanf(
                        line_buf,
                        "%s %d/%d/%d %d/%d/%d %d/%d/%d",
                        t,
                        &face.vertices[0].position_index,
                        &face.vertices[0].texcoord_index,
                        &face.vertices[0].normal_index,

                        &face.vertices[1].position_index,
                        &face.vertices[1].texcoord_index,
                        &face.vertices[1].normal_index,

                        &face.vertices[2].position_index,
                        &face.vertices[2].texcoord_index,
                        &face.vertices[2].normal_index);
                }
                u64 group_index = darray_length(groups) - 1;
                darray_push(groups[group_index].faces, face);
            } break;
            case 'u': {
                // Any time there is a usemtl, assume a new group.
                // New named group or smoothing group, all faces coming after should be added to it.
                MESH_GROUP_DATA new_group;
                new_group.faces = darray_reserve(MESH_FACE_DATA, 16384);
                darray_push(groups, new_group);

                // usemtl
                // Read the material name.
                char t[8];
                sscanf(line_buf, "%s %s", t, material_names[current_mat_name_count]);
                current_mat_name_count++;
            } break;
            case 'm': {
                // Material library file.
                char substr[7];

                sscanf(
                    line_buf,
                    "%s %s",
                    substr,
                    material_file_name);

                // If found, save off the material file name.
                if (strings_nequali(substr, "mtllib", 6)) {
                    // TODO: verification
                }
            } break;
            case 'g': {
                u64 group_count = darray_length(groups);

                // Process each group as a subobject.
                for (u64 i = 0; i < group_count; ++i) {
                    GEOMETRY_CONFIG new_data = {0};
                    string_ncopy(new_data.name, name, 255);
                    if (i > 0) {
                        string_append_int(new_data.name, new_data.name, i);
                    }
                    string_ncopy(new_data.material_name, material_names[i], 255);

                    process_subobject(positions, normals, tex_coords, groups[i].faces, &new_data);
                    new_data.vertex_count = darray_length(new_data.vertices);
                    new_data.vertex_size = sizeof(Vertex3D);
                    new_data.index_count = darray_length(new_data.indices);
                    new_data.index_size = sizeof(u32);

                    darray_push(*out_geometries_darray, new_data);

                    // Increment the number of objects.
                    darray_destroy(groups[i].faces);
                    yzero_memory(material_names[i], 64);
                }
                current_mat_name_count = 0;
                darray_clear(groups);
                yzero_memory(name, 512);

                // Read the name
                char t[2];
                sscanf(line_buf, "%s %s", t, name);

            } break;
        }

        prev_first_chars[1] = prev_first_chars[0];
        prev_first_chars[0] = first_char;
    }  // each line

    // Process the remaining group since the last one will not have been trigged
    // by the finding of a new name.
    // Process each group as a subobject.
    u64 group_count = darray_length(groups);
    for (u64 i = 0; i < group_count; ++i) {
        GEOMETRY_CONFIG new_data = {0};
        string_ncopy(new_data.name, name, 255);
        if (i > 0) {
            string_append_int(new_data.name, new_data.name, i);
        }
        string_ncopy(new_data.material_name, material_names[i], 255);

        process_subobject(positions, normals, tex_coords, groups[i].faces, &new_data);
        new_data.vertex_count = darray_length(new_data.vertices);
        new_data.vertex_size = sizeof(Vertex3D);
        new_data.index_count = darray_length(new_data.indices);
        new_data.index_size = sizeof(u32);

        darray_push(*out_geometries_darray, new_data);

        // Increment the number of objects.
        darray_destroy(groups[i].faces);
    }

    darray_destroy(groups);
    darray_destroy(positions);
    darray_destroy(normals);
    darray_destroy(tex_coords);

    if (string_length(material_file_name) > 0) {
        // Load up the material file
        char full_mtl_path[512];
        string_directory_from_path(full_mtl_path, out_y_mesh_filename);
        string_append_string(full_mtl_path, full_mtl_path, material_file_name);

        // Process material library file.
        if (!import_obj_material_library_file(full_mtl_path)) {
            PRINT_ERROR("Error reading obj mtl file.");
        }
    }

    // De-duplicate geometry
    u32 count = darray_length(*out_geometries_darray);
    for (u64 i = 0; i < count; ++i) {
        GEOMETRY_CONFIG* g = &((*out_geometries_darray)[i]);
        PRINT_DEBUG("Geometry de-duplication process starting on geometry object named '%s'...", g->name);

        u32 new_vert_count = 0;
        Vertex3D* unique_verts = 0;
        geometry_deduplicate_vertices(g->vertex_count, g->vertices, g->index_count, g->indices, &new_vert_count, &unique_verts);

        // Destroy the old, large array...
        darray_destroy(g->vertices);

        // And replace with the de-duplicated one.
        g->vertices = unique_verts;
        g->vertex_count = new_vert_count;

        // Take a copy of the indices as a normal, non-darray
        if (g->index_count > 0) {
            u32* indices = yallocate(sizeof(u32) * g->index_count, MEMORY_TAG_ARRAY);
            ycopy_memory(indices, g->indices, sizeof(u32) * g->index_count);
            // Destroy the darray
            darray_destroy(g->indices);
            // Replace with the non-darray version.
            g->indices = indices;

            // Also generate tangents here, this way tangents are also stored in the output file.
            geometry_generate_tangents(g->vertex_count, g->vertices, g->index_count, g->indices);
        }
    }

    // Output a y_mesh file, which will be loaded in the future.
    return write_y_mesh_file(out_y_mesh_filename, name, count, *out_geometries_darray);
}

b8 import_gltf_file(FILE_HANDLE* gltf_file, const char* path, const char* out_y_mesh_filename, GEOMETRY_CONFIG** out_geometries_darray, b8 is_binary) {
    // read the file and parse it.
    if (!gltf_file || !out_y_mesh_filename || !out_geometries_darray) {
        PRINT_ERROR("Invalid parameters passed to import_gltf_file.");
        return false;
    }
    u8 *file_data = 0;
    u64 file_size = 0;
    if (is_binary) {
        if (!filesystem_size(gltf_file, &file_size)) {
            PRINT_ERROR("Failed to read binary glTF file '%s'.", path);
            filesystem_close(gltf_file);
            return false;
        }

        file_data = yallocate(sizeof(u8) * file_size, MEMORY_TAG_ARRAY);
        u64 read_size = 0;
        if (!filesystem_read_all_bytes(gltf_file, file_data, &read_size)) {
            PRINT_ERROR("Failed to read binary glTF file '%s'.", path);
            filesystem_close(gltf_file);
            return false;
        }
    } else {
        // If it's a glTF, we need to read it in a way that cgltf
        // can parse it correctly.
        filesystem_size(gltf_file, &file_size);
        if (file_size == 0) {
            PRINT_ERROR("File '%s' is empty.", path);
            return false;
        }
        file_data = yallocate(file_size, MEMORY_TAG_STRING);
        u64 bytes_read = 0;
        if (!filesystem_read(gltf_file, file_size, file_data, &bytes_read) || bytes_read != file_size) {
            PRINT_ERROR("Failed to read glTF file '%s'.", path);
            filesystem_close(gltf_file);
            yfree(file_data);
            return false;
        }
    }
    
    // Use cgltf to parse the file.
    cgltf_options options = {0};
    cgltf_data* data = 0;
    cgltf_result result = cgltf_parse(
        &options,
        file_data,
        file_size,
        &data);
    if (result != cgltf_result_success) {
        PRINT_ERROR("Failed to parse glTF file '%s'.", path);
        yfree(file_data);
        return false;
    }

    // Load the data.
    result = cgltf_load_buffers(
        &options,
        data,
        path);
    if (result != cgltf_result_success) {
        PRINT_ERROR("Failed to load buffers for glTF file '%s'. error: '%s'", path, cgltf_error_messages[result]);
        cgltf_free(data);
        yfree(file_data);
        return false;
    }

    // Process the data and create geometry configs.
    for (u32 i = 0; i < data->meshes_count; ++i) {
        const cgltf_mesh* mesh = &data->meshes[i];
        GEOMETRY_CONFIG new_data = {0};
        string_ncopy(new_data.name, mesh->name ? mesh->name : "Unnamed Mesh", 255);

        // Process each primitive in the mesh.
        for (u32 j = 0; j < mesh->primitives_count; ++j) {
            const cgltf_primitive* primitive = &mesh->primitives[j];

            // Positions
            Vector3* positions = darray_reserve(Vector3, 16384);

            // Normals
            Vector3* normals = darray_reserve(Vector3, 16384);

            // Texture coordinates
            Vector2* tex_coords = darray_reserve(Vector2, 16384);

            // Faces
            MESH_FACE_DATA* faces = darray_reserve(MESH_FACE_DATA, primitive->indices ? primitive->indices->count / 3 : 16384);

            // Read attributes
            for (u32 k = 0; k < primitive->attributes_count; ++k) {
                const cgltf_attribute* attribute = &primitive->attributes[k];
                const cgltf_accessor* accessor = attribute->data;

                if (attribute->type == cgltf_attribute_type_position) {
                    for (u32 l = 0; l < accessor->count; ++l) {
                        cgltf_float pos_f[3];
                        cgltf_accessor_read_float(accessor, l, pos_f, 3);
                        Vector3 pos = Vector3_create(pos_f[0], pos_f[1], pos_f[2]);
                        darray_push(positions, pos);
                    }
                } else if (attribute->type == cgltf_attribute_type_normal) {
                    for (u32 l = 0; l < accessor->count; ++l) {
                        cgltf_float norm_f[3];
                        cgltf_accessor_read_float(accessor, l, norm_f, 3);
                        Vector3 norm = Vector3_create(norm_f[0], norm_f[1], norm_f[2]);
                        darray_push(normals, norm);
                    }
                } else if (attribute->type == cgltf_attribute_type_texcoord) {
                    for (u32 l = 0; l < accessor->count; ++l) {
                        cgltf_float tex_f[2];
                        cgltf_accessor_read_float(accessor, l, tex_f, 2);
                        Vector2 tex = Vector2_create(tex_f[0], tex_f[1]);
                        darray_push(tex_coords, tex);
                    }
                }
            }
            // Read indices
            if (primitive->indices) {
                const cgltf_accessor* accessor = primitive->indices;
                for (u32 l = 0; l < accessor->count; l += 3) {
                    MESH_FACE_DATA face;
                    cgltf_uint i0, i1, i2;
                    cgltf_accessor_read_uint(accessor, l, &i0, 1);
                    cgltf_accessor_read_uint(accessor, l + 1, &i1, 1);
                    cgltf_accessor_read_uint(accessor, l + 2, &i2, 1);

                    face.vertices[0].position_index = i0 + 1;
                    face.vertices[1].position_index = i1 + 1;
                    face.vertices[2].position_index = i2 + 1;

                    // NOTE: Assumes that if normals/texcoords exist, they are indexed the same as positions.
                    face.vertices[0].normal_index = darray_length(normals) > 0 ? i0 + 1 : 0;
                    face.vertices[1].normal_index = darray_length(normals) > 0 ? i1 + 1 : 0;
                    face.vertices[2].normal_index = darray_length(normals) > 0 ? i2 + 1 : 0;

                    face.vertices[0].texcoord_index = darray_length(tex_coords) > 0 ? i0 + 1 : 0;
                    face.vertices[1].texcoord_index = darray_length(tex_coords) > 0 ? i1 + 1 : 0;
                    face.vertices[2].texcoord_index = darray_length(tex_coords) > 0 ? i2 + 1 : 0;

                    darray_push(faces, face);
                }
            } else {
                // Not indexed. Create faces from the vertex data.
                u32 vertex_count = darray_length(positions);
                for (u32 l = 0; l < vertex_count; l += 3) {
                    MESH_FACE_DATA face;

                    // Position indices
                    face.vertices[0].position_index = l + 1;
                    face.vertices[1].position_index = l + 2;
                    face.vertices[2].position_index = l + 3;

                    // Normal indices
                    if (darray_length(normals) > 0) {
                        face.vertices[0].normal_index = l + 1;
                        face.vertices[1].normal_index = l + 2;
                        face.vertices[2].normal_index = l + 3;
                    } else {
                        face.vertices[0].normal_index = 0;
                        face.vertices[1].normal_index = 0;
                        face.vertices[2].normal_index = 0;
                    }

                    // Texcoord indices
                    if (darray_length(tex_coords) > 0) {
                        face.vertices[0].texcoord_index = l + 1;
                        face.vertices[1].texcoord_index = l + 2;
                        face.vertices[2].texcoord_index = l + 3;
                    } else {
                        face.vertices[0].texcoord_index = 0;
                        face.vertices[1].texcoord_index = 0;
                        face.vertices[2].texcoord_index = 0;
                    }

                    darray_push(faces, face);
                }
            }
            // Process the subobject
            process_subobject(positions, normals, tex_coords, faces, &new_data);
            new_data.vertex_count = darray_length(new_data.vertices);
            new_data.vertex_size = sizeof(Vertex3D);
            new_data.index_count = darray_length(new_data.indices);
            new_data.index_size = sizeof(u32);
            // Set the material name if available
            if (primitive->material && primitive->material->name) {
                string_ncopy(new_data.material_name, primitive->material->name, 255);
            } else {
                string_ncopy(new_data.material_name, "DefaultMaterial", 255);
            }
            // Add the geometry config to the output array.
            darray_push(*out_geometries_darray, new_data);
            // Clean up
            darray_destroy(positions);
            darray_destroy(normals);
            darray_destroy(tex_coords);
            darray_destroy(faces);
        }
    }
    // If there are no geometries, return false.
    if (darray_length(*out_geometries_darray) == 0) {
        PRINT_ERROR("No geometries found in glTF file '%s'.", path);
        cgltf_free(data);
        yfree(file_data);
        return false;
    }

    // De-duplicate geometry and generate tangents
    u32 count = darray_length(*out_geometries_darray);
    for (u64 i = 0; i < count; ++i) {
        GEOMETRY_CONFIG* g = &((*out_geometries_darray)[i]);
        PRINT_DEBUG("Geometry de-duplication process starting on geometry object named '%s'...", g->name);

        u32 new_vert_count = 0;
        Vertex3D* unique_verts = 0;
        geometry_deduplicate_vertices(g->vertex_count, g->vertices, g->index_count, g->indices, &new_vert_count, &unique_verts);

        // Destroy the old, large array...
        darray_destroy(g->vertices);

        // And replace with the de-duplicated one.
        g->vertices = unique_verts;
        g->vertex_count = new_vert_count;

        // Take a copy of the indices as a normal, non-darray
        if (g->index_count > 0) {
            u32* indices = yallocate(sizeof(u32) * g->index_count, MEMORY_TAG_ARRAY);
            ycopy_memory(indices, g->indices, sizeof(u32) * g->index_count);
            // Destroy the darray
            darray_destroy(g->indices);
            // Replace with the non-darray version.
            g->indices = indices;

            // Also generate tangents here, this way tangents are also stored in the output file.
            geometry_generate_tangents(g->vertex_count, g->vertices, g->index_count, g->indices);
        }
    }

    // Process materials
    PRINT_DEBUG("Processing glTF materials...");
    for (u32 i = 0; i < data->materials_count; ++i) {
        const cgltf_material* material = &data->materials[i];

        MATERIAL_CONFIG m;
        yzero_memory(&m, sizeof(MATERIAL_CONFIG));

        // name
        if (material->name) {
            string_ncopy(m.name, material->name, MATERIAL_NAME_MAX_LENGTH);
        } else {
            string_format(m.name, "material_%u", i);
        }

        // shader
        m.shader_name = "shader.builtin.material";

        // diffuse color
        if (material->has_pbr_metallic_roughness) {
            const cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
            m.diffuse_color.r = pbr->base_color_factor[0];
            m.diffuse_color.g = pbr->base_color_factor[1];
            m.diffuse_color.b = pbr->base_color_factor[2];
            // NOTE: This is only used by the color shader, and will set to max_norm by default.
            // Transparency could be added as a material property all its own at a later time.
            //m.diffuse_color.a = pbr->base_color_factor[3];
            m.diffuse_color.a = 1.0f;

            // NOTE: Using metallic-roughness factor as a base for shininess.
            // This will likely need to be adjusted.
            f32 roughness = pbr->roughness_factor;
            // Inversely related.
            m.shiness = 1.0f - roughness;

            if (m.shiness <= 0.0f) {
                m.shiness = 8.0f;
            }

            // Diffuse map
            if (pbr->base_color_texture.texture) {
                const cgltf_image* image = pbr->base_color_texture.texture->image;
                char texture_filename[512];
                if (extract_gltf_texture(image, path, texture_filename)) {
                    string_filename_no_extension_from_path(m.diffuse_map_name, texture_filename);
                } else if (image->uri) {
                    // Fallback to original URI-based approach
                    string_filename_no_extension_from_path(m.diffuse_map_name, image->uri);
                }
            }

            // Specular map (metallic roughness)
            if (pbr->metallic_roughness_texture.texture) {
                const cgltf_image* image = pbr->metallic_roughness_texture.texture->image;
                char texture_filename[512];
                if (extract_gltf_texture(image, path, texture_filename)) {
                    string_filename_no_extension_from_path(m.specular_map_name, texture_filename);
                } else if (image->uri) {
                    // Fallback to original URI-based approach
                    string_filename_no_extension_from_path(m.specular_map_name, image->uri);
                }
            }
        }
        
        // Normal map
        if (material->normal_texture.texture) {
            const cgltf_image* image = material->normal_texture.texture->image;
            char texture_filename[512];
            if (extract_gltf_texture(image, path, texture_filename)) {
                string_filename_no_extension_from_path(m.normal_map_name, texture_filename);
            } else if (image->uri) {
                // Fallback to original URI-based approach
                string_filename_no_extension_from_path(m.normal_map_name, image->uri);
            }
        }

        // Write the material file.
        if (!write_y_material_file(out_y_mesh_filename, &m)) {
            PRINT_WARNING("Failed to write material file for glTF material: '%s'.", m.name);
        }
    }

    // set name
    char name[512];
    string_filename_from_path(name, out_y_mesh_filename);
    // Write the y_mesh file.
    if (!write_y_mesh_file(out_y_mesh_filename, name, darray_length(*out_geometries_darray), *out_geometries_darray)) {
        PRINT_ERROR("Failed to write y_mesh file '%s'.", out_y_mesh_filename);
        cgltf_free(data);
        yfree(file_data);
        return false;
    }
    // Free the cgltf data.
    cgltf_free(data);
    // Now we can free the file data since we're done with everything
    yfree(file_data);
    PRINT_DEBUG("Successfully imported glTF file '%s' and wrote to '%s'.", path, out_y_mesh_filename);
    filesystem_close(gltf_file);
    
    return true;
}

void process_subobject(Vector3* positions, Vector3* normals, Vector2* tex_coords, MESH_FACE_DATA* faces, GEOMETRY_CONFIG* out_data) {
    out_data->indices = darray_create(u32);
    out_data->vertices = darray_create(Vertex3D);
    b8 extent_set = false;
    yzero_memory(&out_data->min_extents, sizeof(Vector3));
    yzero_memory(&out_data->max_extents, sizeof(Vector3));

    u64 face_count = darray_length(faces);
    u64 normal_count = darray_length(normals);
    u64 tex_coord_count = darray_length(tex_coords);

    b8 skip_normals = false;
    b8 skip_tex_coords = false;
    if (normal_count == 0) {
        PRINT_WARNING("No normals are present in this model.");
        skip_normals = true;
    }
    if (tex_coord_count == 0) {
        PRINT_WARNING("No texture coordinates are present in this model.");
        skip_tex_coords = true;
    }
    for (u64 f = 0; f < face_count; ++f) {
        MESH_FACE_DATA face = faces[f];

        // Each vertex
        for (u64 i = 0; i < 3; ++i) {
            MESH_VERTEX_INDEX_DATA index_data = face.vertices[i];
            darray_push(out_data->indices, (u32)(i + (f * 3)));

            Vertex3D vert;

            Vector3 pos = positions[index_data.position_index - 1];
            vert.position = pos;

            // Check extents - min
            if (pos.x < out_data->min_extents.x || !extent_set) {
                out_data->min_extents.x = pos.x;
            }
            if (pos.y < out_data->min_extents.y || !extent_set) {
                out_data->min_extents.y = pos.y;
            }
            if (pos.z < out_data->min_extents.z || !extent_set) {
                out_data->min_extents.z = pos.z;
            }

            // Check extents - max
            if (pos.x > out_data->max_extents.x || !extent_set) {
                out_data->max_extents.x = pos.x;
            }
            if (pos.y > out_data->max_extents.y || !extent_set) {
                out_data->max_extents.y = pos.y;
            }
            if (pos.z > out_data->max_extents.z || !extent_set) {
                out_data->max_extents.z = pos.z;
            }

            extent_set = true;

            if (skip_normals) {
                vert.normal = Vector3_create(0.0f, 0.0f, 1.0f); // Default normal
            } else {
                vert.normal = normals[index_data.normal_index - 1];
            }

            if (skip_tex_coords) {
                vert.texcoord = Vector2_zero();
            } else {
                vert.texcoord = tex_coords[index_data.texcoord_index - 1];
            }

            // TODO: Color. Hardcode to white for now.
            vert.color = Vector4_one();

            darray_push(out_data->vertices, vert);
        }
    }

    // Calculate the center based on the extents.
    for (u8 i = 0; i < 3; ++i) {
        out_data->center.elements[i] = (out_data->min_extents.elements[i] + out_data->max_extents.elements[i]) / 2.0f;
    }
}

// TODO: Load the material library file, and create material definitions from it.
// These definitions should be output to .y_material files. These .y_material files are then
// loaded when the material is acquired on mesh load.
// NOTE: This should eventually account for duplicate materials. When the .y_material
// files are written, if the file already exists the material should have something
// such as a number appended to its name and a warning thrown to the console. The artist
// should make sure material names are unique. When the material is acquired, the _original_
// existing material name would be used, which would visually be wrong and serve as additional
// reinforcement of the message for material uniqueness.
// Material configs should not be returned or used here.
b8 import_obj_material_library_file(const char* mtl_file_path) {
    PRINT_DEBUG("Importing obj .mtl file '%s'...", mtl_file_path);
    // Grab the .mtl file, if it exists, and read the material information.
    FILE_HANDLE mtl_file;
    if (!filesystem_open(mtl_file_path, FILE_MODE_READ, false, &mtl_file)) {
        PRINT_ERROR("Unable to open mtl file: %s", mtl_file_path);
        return false;
    }

    MATERIAL_CONFIG current_config;
    yzero_memory(&current_config, sizeof(current_config));

    b8 hit_name = false;

    char* line = 0;
    char line_buffer[512];
    char* p = &line_buffer[0];
    u64 line_length = 0;
    while (true) {
        if (!filesystem_read_line(&mtl_file, 512, &p, &line_length)) {
            break;
        }
        // Trim the line first.
        line = string_trim(line_buffer);
        line_length = string_length(line);

        // Skip blank lines.
        if (line_length < 1) {
            continue;
        }

        char first_char = line[0];

        switch (first_char) {
            case '#':
                // Skip comments
                continue;
            case 'K': {
                char second_char = line[1];
                switch (second_char) {
                    case 'a':
                    case 'd': {
                        // Ambient/Diffuse color are treated the same at this level.
                        // ambient color is determined by the level.
                        char t[2];
                        sscanf(
                            line,
                            "%s %f %f %f",
                            t,
                            &current_config.diffuse_color.r,
                            &current_config.diffuse_color.g,
                            &current_config.diffuse_color.b);

                        // NOTE: This is only used by the color shader, and will set to max_norm by default.
                        // Transparency could be added as a material property all its own at a later time.
                        current_config.diffuse_color.a = 1.0f;

                    } break;
                    case 's': {
                        // Specular color
                        char t[2];

                        // NOTE: Not using this for now.
                        f32 spec_rubbish = 0.0f;
                        sscanf(
                            line,
                            "%s %f %f %f",
                            t,
                            &spec_rubbish,
                            &spec_rubbish,
                            &spec_rubbish);
                    } break;
                }
            } break;
            case 'N': {
                char second_char = line[1];
                switch (second_char) {
                    case 's': {
                        // Specular exponent
                        char t[2];

                        sscanf(line, "%s %f", t, &current_config.shiness);
                    } break;
                }
            } break;
            case 'm': {
                // map
                char substr[10];
                char texture_file_name[512];

                sscanf(
                    line,
                    "%s %s",
                    substr,
                    texture_file_name);

                //
                if (strings_nequali(substr, "map_Kd", 6)) {
                    // Is a diffuse texture map
                    string_filename_no_extension_from_path(current_config.diffuse_map_name, texture_file_name);
                } else if (strings_nequali(substr, "map_Ks", 6)) {
                    // Is a specular texture map
                    string_filename_no_extension_from_path(current_config.specular_map_name, texture_file_name);
                } else if (strings_nequali(substr, "map_bump", 8)) {
                    // Is a bump texture map
                    string_filename_no_extension_from_path(current_config.normal_map_name, texture_file_name);
                }
            } break;
            case 'b': {
                // Some implementations use 'bump' instead of 'map_bump'.
                char substr[10];
                char texture_file_name[512];

                sscanf(
                    line,
                    "%s %s",
                    substr,
                    texture_file_name);
                if (strings_nequali(substr, "bump", 4)) {
                    // Is a bump (normal) texture map
                    string_filename_no_extension_from_path(current_config.normal_map_name, texture_file_name);
                }
            }
            case 'n': {
                // Some implementations use 'bump' instead of 'map_bump'.
                char substr[10];
                char material_name[512];

                sscanf(
                    line,
                    "%s %s",
                    substr,
                    material_name);
                if (strings_nequali(substr, "newmtl", 6)) {
                    // Is a material name.

                    // NOTE: Hardcoding default material shader name because all objects imported this way
                    // will be treated the same.
                    current_config.shader_name = "shader.builtin.material";
                    // NOTE: Shininess of 0 will cause problems in the shader. Use a default
                    // if this is the case.
                    if (current_config.shiness == 0.0f) {
                        current_config.shiness = 8.0f;
                    }
                    if (hit_name) {
                        //  Write out a y_material file and move on.
                        if (!write_y_material_file(mtl_file_path, &current_config)) {
                            PRINT_ERROR("Unable to write y_material file.");
                            return false;
                        }

                        // Reset the material for the next round.
                        yzero_memory(&current_config, sizeof(current_config));
                    }

                    hit_name = true;

                    string_ncopy(current_config.name, material_name, 256);
                }
            }
        }

    }  // each line

    // Write out the remaining y_material file.
    // NOTE: Hardcoding default material shader name because all objects imported this way
    // will be treated the same.
    current_config.shader_name = "shader.builtin.material";
    // NOTE: Shininess of 0 will cause problems in the shader. Use a default
    // if this is the case.
    if (current_config.shiness == 0.0f) {
        current_config.shiness = 8.0f;
    }
    if (!write_y_material_file(mtl_file_path, &current_config)) {
        PRINT_ERROR("Unable to write y_material file.");
        return false;
    }

    filesystem_close(&mtl_file);
    return true;
}

/**
 * @brief Write out a y material file from config. This gets loaded by name later when the mesh
 * is requested for load.
 *
 * @param mtl_file_path The filepath of the material library file which originally contained the material definition.
 * @param config A pointer to the config to be converted to y_material.
 * @return True on success; otherwise false.
 */
b8 write_y_material_file(const char* mtl_file_path, MATERIAL_CONFIG* config) {
    // NOTE: The .obj file this came from (and resulting .mtl file) sit in the
    // models directory. This moves up a level and back into the materials folder.
    // TODO: Read from config and get an absolute path for output.
    char* format_str = "%s../materials/%s%s";
    FILE_HANDLE f;
    char directory[320];
    string_directory_from_path(directory, mtl_file_path);

    char full_file_path[512];
    string_format(full_file_path, format_str, directory, config->name, ".y_material");
    if (!filesystem_open(full_file_path, FILE_MODE_WRITE, false, &f)) {
        PRINT_ERROR("Error opening material file for writing: '%s'", full_file_path);
        return false;
    }
    PRINT_DEBUG("Writing .y_material file '%s'...", full_file_path);

    char line_buffer[512];
    filesystem_write_line(&f, "#material file");
    filesystem_write_line(&f, "");
    filesystem_write_line(&f, "version=0.1");  // TODO: hardcoded version.
    string_format(line_buffer, "name=%s", config->name);
    filesystem_write_line(&f, line_buffer);
    string_format(line_buffer, "diffuse_color=%.6f %.6f %.6f %.6f", config->diffuse_color.r, config->diffuse_color.g, config->diffuse_color.b, config->diffuse_color.a);
    filesystem_write_line(&f, line_buffer);
    string_format(line_buffer, "shiness=%.6f", config->shiness);
    filesystem_write_line(&f, line_buffer);
    if (config->diffuse_map_name[0]) {
        string_format(line_buffer, "diffuse_map_name=%s", config->diffuse_map_name);
        filesystem_write_line(&f, line_buffer);
    }
    if (config->specular_map_name[0]) {
        string_format(line_buffer, "specular_map_name=%s", config->specular_map_name);
        filesystem_write_line(&f, line_buffer);
    }
    if (config->normal_map_name[0]) {
        string_format(line_buffer, "normal_map_name=%s", config->normal_map_name);
        filesystem_write_line(&f, line_buffer);
    }
    string_format(line_buffer, "shader=%s", config->shader_name);
    filesystem_write_line(&f, line_buffer);

    filesystem_close(&f);

    return true;
}

b8 extract_gltf_texture(const cgltf_image* image, const char* base_path, char* out_filename) {
    if (!image || !base_path || !out_filename) {
        return false;
    }

    // Generate a filename for the texture
    char texture_name[256];
    if (image->name) {
        string_ncopy(texture_name, image->name, 255);
    } else {
        string_format(texture_name, "texture_%p", (void*)image);
    }

    // Determine file extension from mime type
    const char* extension = "png"; // default
    if (image->mime_type) {
        if (strings_equal(image->mime_type, "image/jpeg")) {
            extension = "jpg";
        } else if (strings_equal(image->mime_type, "image/png")) {
            extension = "png";
        } else if (strings_equal(image->mime_type, "image/webp")) {
            extension = "webp";
        }
    }

    // Create the output filename
    string_format(out_filename, "assets/textures/%s.%s", texture_name, extension);

    // If the image has a buffer view (embedded in glb), extract it
    if (image->buffer_view) {
        // Check if the buffer has data loaded
        if (!image->buffer_view->buffer || !image->buffer_view->buffer->data) {
            PRINT_ERROR("Buffer data not loaded for texture: %s", texture_name);
            return false;
        }
        
        // Use cgltf_buffer_view_data to get the correct data pointer
        const u8* data = cgltf_buffer_view_data(image->buffer_view);
        if (!data) {
            PRINT_ERROR("Failed to get buffer view data for texture: %s", texture_name);
            return false;
        }
        
        u64 size = image->buffer_view->size;

        // Create the file
        FILE_HANDLE texture_file;
        if (!filesystem_open(out_filename, FILE_MODE_WRITE, false, &texture_file)) {
            PRINT_ERROR("Failed to create texture file: %s", out_filename);
            return false;
        }

        // Write the raw binary data
        u64 bytes_written = 0;
        if (!filesystem_write(&texture_file, size, data, &bytes_written) || bytes_written != size) {
            PRINT_ERROR("Failed to write texture data to: %s", out_filename);
            filesystem_close(&texture_file);
            return false;
        }

        filesystem_close(&texture_file);
        PRINT_DEBUG("Extracted embedded texture: %s (%llu bytes)", out_filename, size);
        return true;
    }

    // If the image has a URI (external file), copy it
    if (image->uri) {
        char source_path[512];
        string_format(source_path, "%s", image->uri);
        
        // If it's a relative path, make it relative to the gltf file
        // Check if it's not an absolute path or URL
        b8 is_relative = true;
        if (image->uri[0] == '/' || (image->uri[0] && image->uri[1] == ':')) {
            is_relative = false; // Absolute path
        } else {
            // Check for URL schemes
            const char* colon = strchr(image->uri, ':');
            if (colon && (colon - image->uri) < 10) {
                const char* slash = strchr(colon, '/');
                if (slash && slash == colon + 1 && slash[1] == '/') {
                    is_relative = false; // URL
                }
            }
        }
        
        if (is_relative) {
            char gltf_dir[512];
            string_directory_from_path(gltf_dir, base_path);
            string_format(source_path, "%s/%s", gltf_dir, image->uri);
        }

        // Copy the file
        FILE_HANDLE source_file, dest_file;
        if (!filesystem_open(source_path, FILE_MODE_READ, false, &source_file)) {
            PRINT_ERROR("Failed to open source texture: %s", source_path);
            return false;
        }

        if (!filesystem_open(out_filename, FILE_MODE_WRITE, false, &dest_file)) {
            PRINT_ERROR("Failed to create texture file: %s", out_filename);
            filesystem_close(&source_file);
            return false;
        }

        u64 file_size;
        filesystem_size(&source_file, &file_size);
        
        u8* buffer = yallocate(file_size, MEMORY_TAG_ARRAY);
        u64 bytes_read = 0;
        if (!filesystem_read_all_bytes(&source_file, buffer, &bytes_read) || bytes_read != file_size) {
            PRINT_ERROR("Failed to read source texture: %s", source_path);
            yfree(buffer);
            filesystem_close(&source_file);
            filesystem_close(&dest_file);
            return false;
        }

        u64 bytes_written = 0;
        if (!filesystem_write(&dest_file, file_size, buffer, &bytes_written) || bytes_written != file_size) {
            PRINT_ERROR("Failed to write texture data to: %s", out_filename);
            yfree(buffer);
            filesystem_close(&source_file);
            filesystem_close(&dest_file);
            return false;
        }

        yfree(buffer);
        filesystem_close(&source_file);
        filesystem_close(&dest_file);
        PRINT_DEBUG("Copied external texture: %s -> %s", source_path, out_filename);
        return true;
    }

    return false;
}

RESOURCE_LOADER mesh_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_MESH;
    loader.custom_type = 0;
    loader.load = mesh_loader_load;
    loader.unload = mesh_loader_unload;
    loader.type_path = "models";

    return loader;
}
