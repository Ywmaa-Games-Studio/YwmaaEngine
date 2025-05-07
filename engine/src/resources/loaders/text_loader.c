#include "text_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/ymath.h"

#include "io/filesystem.h"

b8 text_loader_load(struct RESOURCE_LOADER* self, const char* name, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, "");

    // TODO: Should be using an allocator here.
    out_resource->full_path = string_duplicate(full_file_path);

    FILE_HANDLE f;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, false, &f)) {
        PRINT_ERROR("text_loader_load - unable to open file for text reading: '%s'.", full_file_path);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&f, &file_size)) {
        PRINT_ERROR("Unable to text read file: %s.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    // TODO: Should be using an allocator here.
    char* resource_data = yallocate(sizeof(char) * file_size, MEMORY_TAG_ARRAY);
    u64 read_size = 0;
    if (!filesystem_read_all_text(&f, resource_data, &read_size)) {
        PRINT_ERROR("Unable to text read file: %s.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    filesystem_close(&f);

    out_resource->data = resource_data;
    out_resource->data_size = read_size;
    out_resource->name = name;

    return true;
}

void text_loader_unload(struct RESOURCE_LOADER* self, RESOURCE* resource) {
    if (!self || !resource) {
        PRINT_WARNING("text_loader_unload called with nullptr for self or resource.");
        return;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length) {
        yfree(resource->full_path, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        yfree(resource->data, MEMORY_TAG_ARRAY);
        resource->data = 0;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

RESOURCE_LOADER text_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_TEXT;
    loader.custom_type = 0;
    loader.load = text_loader_load;
    loader.unload = text_loader_unload;
    loader.type_path = "";

    return loader;
}
