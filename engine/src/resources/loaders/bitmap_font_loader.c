#include "bitmap_font_loader.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"

#include "loader_utils.h"

#include "io/filesystem.h"

#include <stdio.h>  //sscanf

typedef enum E_BITMAP_FONT_FILE_TYPE {
    BITMAP_FONT_FILE_TYPE_NOT_FOUND,
    BITMAP_FONT_FILE_TYPE_YBF,
    BITMAP_FONT_FILE_TYPE_FNT
} E_BITMAP_FONT_FILE_TYPE;

typedef struct SUPPORTED_BITMAP_FONT_FILETYPE {
    char* extension;
    E_BITMAP_FONT_FILE_TYPE type;
    b8 is_binary;
} SUPPORTED_BITMAP_FONT_FILETYPE;

static b8 import_fnt_file(FILE_HANDLE* fnt_file, const char* out_ybf_filename, BITMAP_FONT_RESOURCE_DATA* out_data);
static b8 read_ybf_file(FILE_HANDLE* ybf_file, BITMAP_FONT_RESOURCE_DATA* data);
static b8 write_ybf_file(const char* path, BITMAP_FONT_RESOURCE_DATA* data);

static b8 bitmap_font_loader_load(struct RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource) {
    if (!self || !name || !out_resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    FILE_HANDLE f;
// Supported extensions. Note that these are in order of priority when looked up.
// This is to prioritize the loading of a binary version of the bitmap font, followed by
// importing various types of bitmap fonts to binary types, which would be loaded on the
// next run.
// TODO: Might be good to be able to specify an override to always import (i.e. skip
// binary versions) for debug purposes.
#define SUPPORTED_FILETYPE_COUNT 2
    SUPPORTED_BITMAP_FONT_FILETYPE supported_filetypes[SUPPORTED_FILETYPE_COUNT];
    supported_filetypes[0] = (SUPPORTED_BITMAP_FONT_FILETYPE){".ybf", BITMAP_FONT_FILE_TYPE_YBF, true};
    supported_filetypes[1] = (SUPPORTED_BITMAP_FONT_FILETYPE){".fnt", BITMAP_FONT_FILE_TYPE_FNT, false};

    char full_file_path[512];
    E_BITMAP_FONT_FILE_TYPE type = BITMAP_FONT_FILE_TYPE_NOT_FOUND;
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

    if (type == BITMAP_FONT_FILE_TYPE_NOT_FOUND) {
        PRINT_ERROR("Unable to find bitmap font of supported type called '%s'.", name);
        return false;
    }

    out_resource->full_path = string_duplicate(full_file_path);

    BITMAP_FONT_RESOURCE_DATA resource_data;
    resource_data.data.type = FONT_TYPE_BITMAP;

    b8 result = false;
    switch (type) {
        case BITMAP_FONT_FILE_TYPE_FNT: {
            // Generate the KBF filename.
            char ybf_file_name[512];
            string_format(ybf_file_name, "%s/%s/%s%s", resource_system_base_path(), self->type_path, name, ".ybf");
            result = import_fnt_file(&f, ybf_file_name, &resource_data);
            break;
        }
        case BITMAP_FONT_FILE_TYPE_YBF:
            result = read_ybf_file(&f, &resource_data);
            break;
        case BITMAP_FONT_FILE_TYPE_NOT_FOUND:
            PRINT_ERROR("Unable to find bitmap font of supported type called '%s'.", name);
            result = false;
            break;
    }

    filesystem_close(&f);

    if (!result) {
        PRINT_ERROR("Failed to process bitmap font file '%s'.", full_file_path);
        string_free(out_resource->full_path);
        out_resource->data = 0;
        out_resource->data_size = 0;
        return false;
    }

    out_resource->data = yallocate_aligned(sizeof(BITMAP_FONT_RESOURCE_DATA), 8, MEMORY_TAG_RESOURCE);
    ycopy_memory(out_resource->data, &resource_data, sizeof(BITMAP_FONT_RESOURCE_DATA));
    out_resource->data_size = sizeof(BITMAP_FONT_RESOURCE_DATA);

    return true;
}

static void bitmap_font_loader_unload(struct RESOURCE_LOADER* self, RESOURCE* resource) {
    if (self && resource) {
        if (resource->data) {
            BITMAP_FONT_RESOURCE_DATA* data = (BITMAP_FONT_RESOURCE_DATA*)resource->data;
            if (data->data.glyph_count && data->data.glyphs) {
                yfree(data->data.glyphs);
                data->data.glyphs = 0;
            }

            if (data->data.kerning_count && data->data.kernings) {
                yfree(data->data.kernings);
                data->data.kernings = 0;
            }

            if (data->page_count && data->pages) {
                yfree(data->pages);
                data->pages = 0;
            }

            yfree(resource->data);
            resource->data = 0;
            resource->data_size = 0;
            resource->loader_id = INVALID_ID;

            if (resource->full_path) {
                yfree(resource->full_path);
                resource->full_path = 0;
            }
        }
    }
}

#define VERIFY_LINE(line_type, line_num, expected, actual)                                                                                     \
    if (actual != expected) {                                                                                                                  \
        PRINT_ERROR("Error in file format reading type '%s', line %u. Expected %d element(s) but read %d.", line_type, line_num, expected, actual); \
        return false;                                                                                                                          \
    }

static b8 import_fnt_file(FILE_HANDLE* fnt_file, const char* out_ybf_filename, BITMAP_FONT_RESOURCE_DATA* out_data) {
    yzero_memory(out_data, sizeof(BITMAP_FONT_RESOURCE_DATA));
    char line_buf[512] = "";
    char* p = &line_buf[0];
    u64 line_length = 0;
    u32 line_num = 0;
    u32 glyphs_read = 0;
    u8 pages_read = 0;
    u32 kernings_read = 0;
    while (true) {
        ++line_num;  // Increment the number right away, since most text editors' line display is 1-indexed.
        if (!filesystem_read_line(fnt_file, 511, &p, &line_length)) {
            break;
        }

        // Skip blank lines.
        if (line_length < 1) {
            continue;
        }

        char first_char = line_buf[0];
        switch (first_char) {
            case 'i': {
                // 'info' line

                // NOTE: only extract the face and size, ignore the rest.
                i32 elements_read = sscanf(
                    line_buf,
                    "info face=\"%[^\"]\" size=%u",
                    out_data->data.face,
                    &out_data->data.size);
                VERIFY_LINE("info", line_num, 2, elements_read);
                break;
            }
            case 'c': {
                // 'common', 'char' or 'chars' line
                if (line_buf[1] == 'o') {
                    // common
                    i32 elements_read = sscanf(
                        line_buf,
                        "common lineHeight=%d base=%u scaleW=%d scaleH=%d pages=%d",  // ignore everything else.
                        &out_data->data.line_height,
                        &out_data->data.baseline,
                        &out_data->data.atlas_size_x,
                        &out_data->data.atlas_size_y,
                        &out_data->page_count);

                    VERIFY_LINE("common", line_num, 5, elements_read);

                    // Allocate the pages array.
                    if (out_data->page_count > 0) {
                        if (!out_data->pages) {
                            out_data->pages = yallocate(sizeof(BITMAP_FONT_PAGE) * out_data->page_count, MEMORY_TAG_ARRAY);
                        }
                    } else {
                        PRINT_ERROR("Pages is 0, which should not be possible. Font file reading aborted.");
                        return false;
                    }
                } else if (line_buf[1] == 'h') {
                    if (line_buf[4] == 's') {
                        // chars line
                        i32 elements_read = sscanf(line_buf, "chars count=%u", &out_data->data.glyph_count);
                        VERIFY_LINE("chars", line_num, 1, elements_read);

                        // Allocate the glyphs array.
                        if (out_data->data.glyph_count > 0) {
                            if (!out_data->data.glyphs) {
                                out_data->data.glyphs = yallocate(sizeof(FONT_GLYPH) * out_data->data.glyph_count, MEMORY_TAG_ARRAY);
                            }
                        } else {
                            PRINT_ERROR("Glyph count is 0, which should not be possible. Font file reading aborted.");
                            return false;
                        }
                    } else {
                        // Assume 'char' line
                        FONT_GLYPH* g = &out_data->data.glyphs[glyphs_read];

                        i32 elements_read = sscanf(
                            line_buf,
                            "char id=%d x=%hu y=%hu width=%hu height=%hu xoffset=%hd yoffset=%hd xadvance=%hd page=%hhu chnl=%*u",
                            &g->codepoint,
                            &g->x,
                            &g->y,
                            &g->width,
                            &g->height,
                            &g->x_offset,
                            &g->y_offset,
                            &g->x_advance,
                            &g->page_id);

                        VERIFY_LINE("char", line_num, 9, elements_read);

                        glyphs_read++;
                    }
                } else {
                    // invalid, ignore
                }
                break;
            }
            case 'p': {
                // 'page' line
                BITMAP_FONT_PAGE* page = &out_data->pages[pages_read];
                i32 elements_read = sscanf(
                    line_buf,
                    "page id=%hhi file=\"%[^\"]\"",
                    &page->id,
                    page->file);

                // Strip the extension.
                string_filename_no_extension_from_path(page->file, page->file);

                VERIFY_LINE("page", line_num, 2, elements_read);

                break;
            }
            case 'k': {
                // 'kernings' or 'kerning' line
                if (line_buf[7] == 's') {
                    // Kernings
                    i32 elements_read = sscanf(line_buf, "kernings count=%u", &out_data->data.kerning_count);

                    VERIFY_LINE("kernings", line_num, 1, elements_read);

                    // Allocate kernings array
                    if (!out_data->data.kernings) {
                        out_data->data.kernings = yallocate(sizeof(FONT_KERNING) * out_data->data.kerning_count, MEMORY_TAG_ARRAY);
                    }
                } else if (line_buf[7] == ' ') {
                    // Kerning record
                    FONT_KERNING* k = &out_data->data.kernings[kernings_read];
                    i32 elements_read = sscanf(
                        line_buf,
                        "kerning first=%i  second=%i amount=%hi",
                        &k->codepoint_0,
                        &k->codepoint_1,
                        &k->amount);

                    VERIFY_LINE("kerning", line_num, 3, elements_read);
                }
                break;
            }
            default:
                // Skip the line.
                break;
        }
    }

    // Now write the binary bitmap font file.
    return write_ybf_file(out_ybf_filename, out_data);
}

static b8 read_ybf_file(FILE_HANDLE* file, BITMAP_FONT_RESOURCE_DATA* data) {
    yzero_memory(data, sizeof(BITMAP_FONT_RESOURCE_DATA));

    u64 bytes_read = 0;
    u32 read_size = 0;

    // Write the resource header first.
    RESOURCE_HEADER header;
    read_size = sizeof(RESOURCE_HEADER);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &header, &bytes_read), file);

    // Verify header contents.
    if (header.magic_number != RESOURCE_MAGIC && header.resource_type == RESOURCE_TYPE_BITMAP_FONT) {
        PRINT_ERROR("KBF file header is invalid and cannot be read.");
        filesystem_close(file);
        return false;
    }

    // TODO: read in/process file version.

    // Length of face string.
    u32 face_length;
    read_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &face_length, &bytes_read), file);

    // Face string.
    read_size = sizeof(char) * face_length + 1;
    CLOSE_IF_FAILED(filesystem_read(file, read_size, data->data.face, &bytes_read), file);
    // Ensure zero-termination
    data->data.face[face_length] = 0;

    // Font size
    read_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.size, &bytes_read), file);

    // Line height
    read_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.line_height, &bytes_read), file);

    // Baseline
    read_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.baseline, &bytes_read), file);

    // scale x
    read_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.atlas_size_x, &bytes_read), file);

    // scale y
    read_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.atlas_size_y, &bytes_read), file);

    // page count
    read_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->page_count, &bytes_read), file);

    // Allocate pages array
    data->pages = yallocate(sizeof(BITMAP_FONT_PAGE) * data->page_count, MEMORY_TAG_ARRAY);

    // Read pages
    for (u32 i = 0; i < data->page_count; ++i) {
        // Page id
        read_size = sizeof(i8);
        CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->pages[i].id, &bytes_read), file);

        // File name length
        u32 filename_length = string_length(data->pages[i].file) + 1;
        read_size = sizeof(u32);
        CLOSE_IF_FAILED(filesystem_read(file, read_size, &filename_length, &bytes_read), file);

        // The file name
        read_size = sizeof(char) * filename_length + 1;
        CLOSE_IF_FAILED(filesystem_read(file, read_size, data->pages[i].file, &bytes_read), file);
        // Ensure zero-termination.
        data->pages[i].file[filename_length] = 0;
    }

    // Glyph count
    read_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.glyph_count, &bytes_read), file);

    // Allocate glyphs array
    data->data.glyphs = yallocate(sizeof(FONT_GLYPH) * data->data.glyph_count, MEMORY_TAG_ARRAY);

    // Read glyphs. These don't contain any strings, so can just read in the entire block.
    read_size = sizeof(FONT_GLYPH) * data->data.glyph_count;
    CLOSE_IF_FAILED(filesystem_read(file, read_size, data->data.glyphs, &bytes_read), file);

    // Kerning Count
    read_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_read(file, read_size, &data->data.kerning_count, &bytes_read), file);

    // It's possible to have a font with no kernings. If this is the
    // case, nothing can be read. This is also why this is done last.
    if (data->data.kerning_count > 0) {
        // Allocate kernings array
        data->data.kernings = yallocate(sizeof(FONT_GLYPH) * data->data.kerning_count, MEMORY_TAG_ARRAY);

        // No strings for kernings, so read the entire block.
        read_size = sizeof(FONT_KERNING) * data->data.kerning_count;
        CLOSE_IF_FAILED(filesystem_read(file, read_size, data->data.kernings, &bytes_read), file);
    }

    // Done, close the file.
    filesystem_close(file);

    return true;
}

static b8 write_ybf_file(const char* path, BITMAP_FONT_RESOURCE_DATA* data) {
    // Header info first
    FILE_HANDLE file;
    if (!filesystem_open(path, FILE_MODE_WRITE, true, &file)) {
        PRINT_ERROR("Failed to open file for writing: %s", path);
        return false;
    }

    u64 bytes_written = 0;
    u32 write_size = 0;

    // Write the resource header first.
    RESOURCE_HEADER header;
    header.magic_number = RESOURCE_MAGIC;
    header.resource_type = RESOURCE_TYPE_BITMAP_FONT;
    header.version = 0x01U;  // Version 1 for now.
    header.reserved = 0;
    write_size = sizeof(RESOURCE_HEADER);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &header, &bytes_written), &file);

    // Length of face string.
    u32 face_length = string_length(data->data.face);
    write_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &face_length, &bytes_written), &file);

    // Face string.
    write_size = sizeof(char) * face_length + 1;
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, data->data.face, &bytes_written), &file);

    // Font size
    write_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.size, &bytes_written), &file);

    // Line height
    write_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.line_height, &bytes_written), &file);

    // Baseline
    write_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.baseline, &bytes_written), &file);

    // scale x
    write_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.atlas_size_x, &bytes_written), &file);

    // scale y
    write_size = sizeof(i32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.atlas_size_y, &bytes_written), &file);

    // page count
    write_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->page_count, &bytes_written), &file);

    // Write pages
    for (u32 i = 0; i < data->page_count; ++i) {
        // Page id
        write_size = sizeof(i8);
        CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->pages[i].id, &bytes_written), &file);

        // File name length
        u32 filename_length = string_length(data->pages[i].file) + 1;
        write_size = sizeof(u32);
        CLOSE_IF_FAILED(filesystem_write(&file, write_size, &filename_length, &bytes_written), &file);

        // The file name
        write_size = sizeof(char) * filename_length + 1;
        CLOSE_IF_FAILED(filesystem_write(&file, write_size, data->pages[i].file, &bytes_written), &file);
    }

    // Glyph count
    write_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.glyph_count, &bytes_written), &file);

    // Write glyphs. These don't contain any strings, so can just write out the entire block.
    write_size = sizeof(FONT_GLYPH) * data->data.glyph_count;
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, data->data.glyphs, &bytes_written), &file);

    // Kerning Count
    write_size = sizeof(u32);
    CLOSE_IF_FAILED(filesystem_write(&file, write_size, &data->data.kerning_count, &bytes_written), &file);

    // It's possible to have a font with no kernings. If this is the
    // case, nothing can be written. This is also why this is done last.
    if (data->data.kerning_count > 0) {
        // No strings for kernings, so write the entire block.
        write_size = sizeof(FONT_KERNING) * data->data.kerning_count;
        CLOSE_IF_FAILED(filesystem_write(&file, write_size, data->data.kernings, &bytes_written), &file);
    }

    // Done, close the file.
    filesystem_close(&file);

    return true;
}

RESOURCE_LOADER bitmap_font_resource_loader_create(void) {
    RESOURCE_LOADER loader;
    loader.type = RESOURCE_TYPE_BITMAP_FONT;
    loader.custom_type = 0;
    loader.load = bitmap_font_loader_load;
    loader.unload = bitmap_font_loader_unload;
    loader.type_path = "fonts";

    return loader;
}
