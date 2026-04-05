#include "image_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "io/filesystem.h"
#include "loader_utils.h"

// TODO: resource loader.
#define STB_IMAGE_IMPLEMENTATION
// Use our own filesystem.
#define STBI_NO_STDIO
#include "vendor/stb_image.h"

static b8 image_loader_load(RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    IMAGE_RESOURCE_PARAMS* typed_params = (IMAGE_RESOURCE_PARAMS*)params;

    char* format_str = "%s/%s/%s%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load_thread(typed_params->flip_y);
    char full_file_path[512];

// Try different extensions.
#define IMAGE_EXTENSION_COUNT 4
    b8 found = false;
    char* extensions[IMAGE_EXTENSION_COUNT] = {".tga", ".png", ".jpg", ".bmp"};
    for (u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i) {
        string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, extensions[i]);
        if (filesystem_exists(full_file_path)) {
            found = true;
            break;
        }
    }

    // Take a copy of the resource full path and name first.
    out_resource->full_path = string_duplicate(full_file_path);
    out_resource->name = name;

    if (!found) {
        PRINT_ERROR("Image resource loader failed find file '%s' or with any supported extension.", full_file_path);
        return false;
    }

    FILE_HANDLE f;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, true, &f)) {
        PRINT_ERROR("Unable to read file: %s.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&f, &file_size)) {
        PRINT_ERROR("Unable to get size of file: %s.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    i32 width;
    i32 height;
    i32 channel_count;

    u8* raw_data = yallocate(file_size, MEMORY_TAG_TEXTURE);
    if (!raw_data) {
        PRINT_ERROR("Unable to read file '%s'.", full_file_path);
        filesystem_close(&f);
        return false;
    }

    u64 bytes_read = 0;
    b8 read_result = filesystem_read_all_bytes(&f, raw_data, &bytes_read);
    filesystem_close(&f);

    if (!read_result) {
        PRINT_ERROR("Unable to read file: '%s'", full_file_path);
        return false;
    }

    if (bytes_read != file_size) {
        PRINT_ERROR("File size if %llu does not match expected: %llu", bytes_read, file_size);
        return false;
    }

    u8* data = stbi_load_from_memory(raw_data, file_size, &width, &height, &channel_count, required_channel_count);

    if (!data) {
        PRINT_ERROR("Image resource loader failed to load file '%s'.", full_file_path);
        return false;
    }

    yfree(raw_data);

    // TODO: Should be using an allocator here.
    IMAGE_RESOURCE_DATA* resource_data = yallocate_aligned(sizeof(IMAGE_RESOURCE_DATA), 8, MEMORY_TAG_TEXTURE);
    resource_data->pixels = data;
    resource_data->width = width;
    resource_data->height = height;
    resource_data->channel_count = required_channel_count;

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(IMAGE_RESOURCE_DATA);

    return true;
}

static void image_loader_unload(RESOURCE_LOADER* self, RESOURCE* resource) {
    stbi_image_free(((IMAGE_RESOURCE_DATA*)resource->data)->pixels);
    if (!resource_unload(self, resource, MEMORY_TAG_TEXTURE)) {
        PRINT_WARNING("image_loader_unload called with nullptr for self or resource.");
        return;
    }
}

RESOURCE_LOADER image_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_IMAGE;
    loader.custom_type = 0;
    loader.load = image_loader_load;
    loader.unload = image_loader_unload;
    loader.type_path = "textures";

    return loader;
}
