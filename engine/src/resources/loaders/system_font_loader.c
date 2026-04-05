#include "system_font_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/ymath.h"
#include "loader_utils.h"
#include "data_structures/darray.h"

#include "io/filesystem.h"

typedef enum system_font_file_type {
    SYSTEM_FONT_FILE_TYPE_NOT_FOUND,
    SYSTEM_FONT_FILE_TYPE_YSF,
    SYSTEM_FONT_FILE_TYPE_FONTCONFIG
} system_font_file_type;

typedef struct supported_system_font_filetype {
    char* extension;
    system_font_file_type type;
    b8 is_binary;
} supported_system_font_filetype;

static b8 import_fontconfig_file(FILE_HANDLE* f, const char* type_path, const char* out_ysf_filename, SYSTEM_FONT_RESOURCE_DATA* out_resource);
static b8 read_ysf_file(FILE_HANDLE* file, SYSTEM_FONT_RESOURCE_DATA* data);
static b8 write_ysf_file(const char* out_ysf_filename, SYSTEM_FONT_RESOURCE_DATA* resource);

static b8 system_font_loader_load(struct RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    FILE_HANDLE f;
    // Supported extensions. Note that these are in order of priority when looked up.
    // This is to prioritize the loading of a binary version of the system font, followed by
    // importing various types of system fonts to binary types, which would be loaded on the
    // next run.
    // TODO: Might be good to be able to specify an override to always import (i.e. skip
    // binary versions) for debug purposes.
#define SUPPORTED_FILETYPE_COUNT 2
    supported_system_font_filetype supported_filetypes[SUPPORTED_FILETYPE_COUNT];
    supported_filetypes[0] = (supported_system_font_filetype){".ysf", SYSTEM_FONT_FILE_TYPE_YSF, true};
    supported_filetypes[1] = (supported_system_font_filetype){".fontcfg", SYSTEM_FONT_FILE_TYPE_FONTCONFIG, false};

    char full_file_path[512];
    system_font_file_type type = SYSTEM_FONT_FILE_TYPE_NOT_FOUND;
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

    if (type == SYSTEM_FONT_FILE_TYPE_NOT_FOUND) {
        PRINT_ERROR("Unable to find system font of supported type called '%s'.", name);
        return false;
    }

    out_resource->full_path = string_duplicate(full_file_path);

    SYSTEM_FONT_RESOURCE_DATA resource_data;

    b8 result = false;
    switch (type) {
        case SYSTEM_FONT_FILE_TYPE_FONTCONFIG: {
            // Generate the ysf filename.
            char ysf_file_name[512];
            string_format(ysf_file_name, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".ysf");
            result = import_fontconfig_file(&f, self->type_path, ysf_file_name, &resource_data);
            break;
        }
        case SYSTEM_FONT_FILE_TYPE_YSF:
            result = read_ysf_file(&f, &resource_data);
            break;
        case SYSTEM_FONT_FILE_TYPE_NOT_FOUND:
            PRINT_ERROR("Unable to find system font of supported type called '%s'.", name);
            result = false;
            break;
    }

    filesystem_close(&f);

    if (!result) {
        PRINT_ERROR("Failed to process system font file '%s'.", full_file_path);
        out_resource->data = 0;
        out_resource->data_size = 0;
        return false;
    }

    out_resource->data = yallocate_aligned(sizeof(SYSTEM_FONT_RESOURCE_DATA), 8, MEMORY_TAG_RESOURCE);
    ycopy_memory(out_resource->data, &resource_data, sizeof(SYSTEM_FONT_RESOURCE_DATA));
    out_resource->data_size = sizeof(SYSTEM_FONT_RESOURCE_DATA);

    return true;
}

static b8 import_fontconfig_file(FILE_HANDLE* f, const char* type_path, const char* out_ysf_filename, SYSTEM_FONT_RESOURCE_DATA* out_resource) {
    out_resource->fonts = darray_create(SYSTEM_FONT_FACE);
    out_resource->binary_size = 0;
    out_resource->font_binary = 0;

    // Read each line of the file.
    char line_buf[512] = "";
    char* p = &line_buf[0];
    u64 line_length = 0;
    u32 line_number = 1;
    while (filesystem_read_line(f, 511, &p, &line_length)) {
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
            PRINT_WARNING("Potential formatting issue found in file: '=' token not found. Skipping line %u.", line_number);
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
        } else if (strings_equali(trimmed_var_name, "file")) {

            char* format_str = "%s/%s/%s";
            char full_file_path[512];
            string_format(full_file_path, format_str, resource_system_base_path(), type_path, trimmed_value);

            // Open and read the font file as binary, and save into an allocated
            // buffer on the resource itself.
            FILE_HANDLE font_binary_handle;
            if (!filesystem_open(full_file_path, FILE_MODE_READ, true, &font_binary_handle)) {
                PRINT_ERROR("Unable to open binary font file. Load process failed.");
                return false;
            }
            u64 file_size;
            if (!filesystem_size(&font_binary_handle, &file_size)) {
                PRINT_ERROR("Unable to get file size of binary font file. Load process failed.");
                return false;
            }
            out_resource->font_binary = yallocate(file_size, MEMORY_TAG_RESOURCE);
            if (!filesystem_read_all_bytes(&font_binary_handle, out_resource->font_binary, &out_resource->binary_size)) {
                PRINT_ERROR("Unable to perform binary read on font file. Load process failed.");
                return false;
            }

            // Might still work anyway, so continue.
            if (out_resource->binary_size != file_size) {
                PRINT_WARNING("Mismatch between filesize and bytes read in font file. File may be corrupt.");
            }

            filesystem_close(&font_binary_handle);
        } else if (strings_equali(trimmed_var_name, "face")) {
            // Read in the font face and store it for later.
            SYSTEM_FONT_FACE new_face;
            string_ncopy(new_face.name, trimmed_value, 255);
            darray_push(out_resource->fonts, new_face);
        }

        // Clear the line buffer.
        yzero_memory(line_buf, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(f);

    // Check here to make sure a binary was loaded, and at least one font face was found.
    if (!out_resource->font_binary || darray_length(out_resource->fonts) < 1) {
        PRINT_ERROR("Font configuration did not provide a binary and at least one font face. Load process failed.");
        return false;
    }

    return write_ysf_file(out_ysf_filename, out_resource);
}

static b8 read_ysf_file(FILE_HANDLE* file, SYSTEM_FONT_RESOURCE_DATA* data) {
    yzero_memory(data, sizeof(SYSTEM_FONT_RESOURCE_DATA));

    u64 bytes_read = 0;
    u32 read_size = 0;

    // Write the resource header first.
    RESOURCE_HEADER header;
    read_size = sizeof(RESOURCE_HEADER);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &header, &bytes_read), file);

    // Verify header contents.
    if (header.magic_number != RESOURCE_MAGIC && header.resource_type == RESOURCE_TYPE_SYSTEM_FONT) {
        PRINT_ERROR("KSF file header is invalid and cannot be read.");
        filesystem_close(file);
        return false;
    }

    // TODO: read in/process file version.

    // Size of font binary.
    read_size = sizeof(u64);
    CLOSE_IF_FAILED(filesystem_write(file, read_size, &data->binary_size, &bytes_read), file);

    // The font binary itself
    read_size = data->binary_size;
    CLOSE_IF_FAILED(filesystem_write(file, read_size, &data->font_binary, &bytes_read), file);

    // The number of fonts
    u32 font_count = darray_length(data->fonts);
    read_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_write(file, read_size, &font_count, &bytes_read), file);

    // Iterate faces metadata and output that as well.
    for (u32 i = 0; i < font_count; ++i) {
        // Length of face name string.
        u32 face_length = string_length(data->fonts[i].name);
        read_size = sizeof(u32);
        CLOSE_IF_FAILED(filesystem_write(file, read_size, &face_length, &bytes_read), file);

        // Face string.
        read_size = sizeof(char) * face_length + 1;
        CLOSE_IF_FAILED(filesystem_write(file, read_size, data->fonts[i].name, &bytes_read), file);
    }

    return true;
}

static b8 write_ysf_file(const char* out_ysf_filename, SYSTEM_FONT_RESOURCE_DATA* resource) {
    // TODO: Implement binary system font.
    return true;
}

static void system_font_loader_unload(struct RESOURCE_LOADER* self, RESOURCE* resource) {
    if (self && resource) {
        SYSTEM_FONT_RESOURCE_DATA* data = (SYSTEM_FONT_RESOURCE_DATA*)resource->data;
        if (data->fonts) {
            darray_destroy(data->fonts);
            data->fonts = 0;
        }

        if (data->font_binary) {
            yfree(data->font_binary);
            data->font_binary = 0;
            data->binary_size = 0;
        }
    }
}

RESOURCE_LOADER system_font_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_SYSTEM_FONT;
    loader.custom_type = 0;
    loader.load = system_font_loader_load;
    loader.unload = system_font_loader_unload;
    loader.type_path = "fonts";

    return loader;
}
