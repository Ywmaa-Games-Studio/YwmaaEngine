#include "binary_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/ymath.h"

#include "io/filesystem.h"
#include "loader_utils.h"

static b8 binary_loader_load(RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, "");

    FILE_HANDLE f;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, true, &f)) {
        PRINT_ERROR("binary_loader_load - unable to open file for binary reading: '%s'.", full_file_path);
        return false;
    }

    // TODO: Should be using an allocator here.
    out_resource->full_path = string_duplicate(full_file_path);

    u64 file_size = 0;
    if (!filesystem_size(&f, &file_size)) {
        PRINT_ERROR("Unable to binary read file: %s.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    // TODO: Should be using an allocator here.
    u8* resource_data = yallocate(sizeof(u8) * file_size, MEMORY_TAG_ARRAY);
    u64 read_size = 0;
    if (!filesystem_read_all_bytes(&f, resource_data, &read_size)) {
        PRINT_ERROR("Unable to binary read file: %s.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    filesystem_close(&f);

    out_resource->data = resource_data;
    out_resource->data_size = read_size;
    out_resource->name = name;

    return true;
}

static void binary_loader_unload(RESOURCE_LOADER* self, RESOURCE* resource) {
    if (!resource_unload(self, resource, MEMORY_TAG_ARRAY)) {
        PRINT_WARNING("binary_loader_unload called with nullptr for self or resource.");
        return;
    }
}

RESOURCE_LOADER binary_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_BINARY;
    loader.custom_type = 0;
    loader.load = binary_loader_load;
    loader.unload = binary_loader_unload;
    loader.type_path = "";

    return loader;
}
