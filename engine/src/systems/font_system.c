#include "font_system.h"

#include "core/ymemory.h"
#include "core/logger.h"
#include "core/ystring.h"
#include "data_structures/darray.h"
#include "data_structures/hashtable.h"
#include "resources/resource_types.h"
#include "resources/ui_text.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"
#include "systems/resource_system.h"

// For system fonts.
#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"

typedef struct BITMAP_FONT_INTERNAL_DATA {
    RESOURCE loaded_resource;
    // Casted pointer to resource data for convenience.
    BITMAP_FONT_RESOURCE_DATA* resource_data;
} BITMAP_FONT_INTERNAL_DATA;

typedef struct SYSTEM_FONT_VARIANT_DATA {
    // darray
    i32* codepoints;
    f32 scale;
} SYSTEM_FONT_VARIANT_DATA;

typedef struct BITMAP_FONT_LOOKUP {
    u16 id;
    u16 reference_count;
    BITMAP_FONT_INTERNAL_DATA font;
} BITMAP_FONT_LOOKUP;

typedef struct SYSTEM_FONT_LOOKUP {
    u16 id;
    u16 reference_count;
    // darray
    FONT_DATA* size_variants;
    // A copy of all this is kept for each for convenience.
    u64 binary_size;
    char* face;
    void* font_binary;
    i32 offset;
    i32 index;
    stbtt_fontinfo info;
} SYSTEM_FONT_LOOKUP;

typedef struct FONT_SYSTEM_STATE {
    FONT_SYSTEM_CONFIG config;
    HASHTABLE bitmap_font_lookup;
    HASHTABLE system_font_lookup;
    BITMAP_FONT_LOOKUP* bitmap_fonts;
    SYSTEM_FONT_LOOKUP* system_fonts;
    void* bitmap_hashtable_block;
    void* system_hashtable_block;
} FONT_SYSTEM_STATE;

b8 setup_font_data(FONT_DATA* font);
void cleanup_font_data(FONT_DATA* font);
b8 create_system_font_variant(SYSTEM_FONT_LOOKUP* lookup, u16 size, const char* font_name, FONT_DATA* out_variant);
b8 rebuild_system_font_variant_atlas(SYSTEM_FONT_LOOKUP* lookup, FONT_DATA* variant);
b8 verify_system_font_size_variant(SYSTEM_FONT_LOOKUP* lookup, FONT_DATA* variant, const char* text);

static FONT_SYSTEM_STATE* state_ptr;

b8 font_system_init(u64* memory_requirement, void* memory, void* config) {
    FONT_SYSTEM_CONFIG* typed_config = (FONT_SYSTEM_CONFIG*)config;
    if (typed_config->max_bitmap_font_count == 0 || typed_config->max_system_font_count == 0) {
        PRINT_ERROR("font_system_init - config.max_bitmap_font_count and config.max_system_font_count must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then blocks for arrays, then blocks for hashtables.
    u64 struct_requirement = sizeof(FONT_SYSTEM_STATE);
    u64 bmp_array_requirement = sizeof(BITMAP_FONT_LOOKUP) * typed_config->max_bitmap_font_count;
    u64 sys_array_requirement = sizeof(SYSTEM_FONT_LOOKUP) * typed_config->max_system_font_count;
    u64 bmp_hashtable_requirement = sizeof(u16) * typed_config->max_bitmap_font_count;
    u64 sys_hashtable_requirement = sizeof(u16) * typed_config->max_system_font_count;
    *memory_requirement = struct_requirement + bmp_array_requirement + sys_array_requirement + bmp_hashtable_requirement + sys_hashtable_requirement;

    if (!memory) {
        return true;
    }

    state_ptr = (FONT_SYSTEM_STATE*)memory;
    state_ptr->config = *typed_config;

    // The array blocks are after the state. Already allocated, so just set the pointer.
    void* bmp_array_block = (void*)(((u8*)memory) + struct_requirement);
    void* sys_array_block = (void*)(((u8*)bmp_array_block) + bmp_array_requirement);

    state_ptr->bitmap_fonts = bmp_array_block;
    state_ptr->system_fonts = sys_array_block;

    // Hashtable blocks are after arrays.
    void* bmp_hashtable_block = (void*)(((u8*)sys_array_block) + sys_array_requirement);
    void* sys_hashtable_block = (void*)(((u8*)bmp_hashtable_block) + bmp_hashtable_requirement);

    // Create hashtables for font lookups.
    hashtable_create(sizeof(u16), state_ptr->config.max_bitmap_font_count, bmp_hashtable_block, false, &state_ptr->bitmap_font_lookup);
    hashtable_create(sizeof(u16), state_ptr->config.max_system_font_count, sys_hashtable_block, false, &state_ptr->system_font_lookup);

    // Fill both hashtables with invalid references to use as a default.
    u16 invalid_id = INVALID_ID_U16;
    hashtable_fill(&state_ptr->bitmap_font_lookup, &invalid_id);
    hashtable_fill(&state_ptr->system_font_lookup, &invalid_id);

    // Invalidate all entries in both arrays.
    u32 count = state_ptr->config.max_bitmap_font_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->bitmap_fonts[i].id = INVALID_ID_U16;
        state_ptr->bitmap_fonts[i].reference_count = 0;
    }
    count = state_ptr->config.max_system_font_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->system_fonts[i].id = INVALID_ID_U16;
        state_ptr->system_fonts[i].reference_count = 0;
    }

    // Load up any default fonts.
    // Bitmap fonts.
    for (u32 i = 0; i < state_ptr->config.default_bitmap_font_count; ++i) {
        if (!font_system_load_bitmap_font(&state_ptr->config.bitmap_font_configs[i])) {
            PRINT_ERROR("Failed to load bitmap font: %s", state_ptr->config.bitmap_font_configs[i].name);
        }
    }

    // System fonts.
    for (u32 i = 0; i < state_ptr->config.default_system_font_count; ++i) {
        if (!font_system_load_system_font(&state_ptr->config.system_font_configs[i])) {
            PRINT_ERROR("Failed to load system font: %s", state_ptr->config.system_font_configs[i].name);
        }
    }

    return true;
}

void font_system_shutdown(void* memory) {
    if (memory) {
        // Cleanup bitmap fonts.
        for (u16 i = 0; i < state_ptr->config.max_bitmap_font_count; ++i) {
            if (state_ptr->bitmap_fonts[i].id != INVALID_ID_U16) {
                FONT_DATA* data = &state_ptr->bitmap_fonts[i].font.resource_data->data;
                cleanup_font_data(data);
                state_ptr->bitmap_fonts[i].id = INVALID_ID_U16;
            }
        }

        // Cleanup system fonts.
        for (u16 i = 0; i < state_ptr->config.max_system_font_count; ++i) {
            if (state_ptr->system_fonts[i].id != INVALID_ID_U16) {
                // Cleanup each variant.
                u32 variant_count = darray_length(state_ptr->system_fonts[i].size_variants);
                for (u32 j = 0; j < variant_count; ++j) {
                    FONT_DATA* data = &state_ptr->system_fonts[i].size_variants[j];
                    cleanup_font_data(data);
                }
                state_ptr->bitmap_fonts[i].id = INVALID_ID_U16;

                darray_destroy(state_ptr->system_fonts[i].size_variants);
                state_ptr->system_fonts[i].size_variants = 0;
            }
        }
    }
}

b8 font_system_load_system_font(SYSTEM_FONT_CONFIG* config) {
    // For system fonts, they can actually contain multiple fonts. For this reason,
    // a copy of the resource's data will be held in each resulting variant, and the
    // resource itself will be released.
    RESOURCE loaded_resource;
    if (!resource_system_load(config->resource_name, RESOURCE_TYPE_SYSTEM_FONT, 0, &loaded_resource)) {
        PRINT_ERROR("Failed to load system font.");
        return false;
    }

    // Keep a casted pointer to the resource data for convenience.
    SYSTEM_FONT_RESOURCE_DATA* resource_data = (SYSTEM_FONT_RESOURCE_DATA*)loaded_resource.data;

    // Loop through the faces and create one lookup for each, as well as a default size
    // variant for each lookup.
    u32 font_face_count = darray_length(resource_data->fonts);
    for (u32 i = 0; i < font_face_count; ++i) {
        SYSTEM_FONT_FACE* face = &resource_data->fonts[i];

        // Make sure a font with this name doesn't already exist.
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->system_font_lookup, face->name, &id)) {
            PRINT_ERROR("Hashtable lookup failed. Font will not be loaded.");
            return false;
        }
        if (id != INVALID_ID_U16) {
            PRINT_WARNING("A font named '%s' already exists and will not be loaded again.", config->name);
            // Not a hard error, return success since it already exists and can be used.
            return true;
        }

        // Get a new id
        for (u16 j = 0; j < state_ptr->config.max_system_font_count; ++j) {
            if (state_ptr->system_fonts[j].id == INVALID_ID_U16) {
                id = j;
                break;
            }
        }
        if (id == INVALID_ID_U16) {
            PRINT_ERROR("No space left to allocate a new font. Increase maximum number allowed in font system config.");
            return false;
        }

        // Obtain the lookup.
        SYSTEM_FONT_LOOKUP* lookup = &state_ptr->system_fonts[id];
        lookup->binary_size = resource_data->binary_size;
        lookup->font_binary = resource_data->font_binary;
        lookup->face = string_duplicate(face->name);
        lookup->index = i;
        // To hold the size variants.
        lookup->size_variants = darray_create(FONT_DATA);

        // The offset
        lookup->offset = stbtt_GetFontOffsetForIndex(lookup->font_binary, i);
        i32 result = stbtt_InitFont(&lookup->info, lookup->font_binary, lookup->offset);
        if (result == 0) {
            // Zero indicates failure.
            PRINT_ERROR("Failed to init system font %s at index %i.", loaded_resource.full_path, i);
            return false;
        }

        // Create a default size variant.
        FONT_DATA variant;
        if (!create_system_font_variant(lookup, config->default_size, face->name, &variant)) {
            PRINT_ERROR("Failed to create variant: %s, index %i", face->name, i);
            continue;
        }

        // Also perform setup for the variant
        if (!setup_font_data(&variant)) {
            PRINT_ERROR("Failed to setup font data");
            continue;
        }

        // Add to the lookup's size variants.
        darray_push(lookup->size_variants, variant);

        // Set the entry id here last before updating the hashtable.
        lookup->id = id;
        if (!hashtable_set(&state_ptr->system_font_lookup, face->name, &id)) {
            PRINT_ERROR("Hashtable set failed on font load.");
            return false;
        }
    }

    return true;
}

b8 font_system_load_bitmap_font(BITMAP_FONT_CONFIG* config) {
    // Make sure a font with this name doesn't already exist.
    u16 id = INVALID_ID_U16;
    if (!hashtable_get(&state_ptr->bitmap_font_lookup, config->name, &id)) {
        PRINT_ERROR("Hashtable lookup failed. Font will not be loaded.");
        return false;
    }
    if (id != INVALID_ID_U16) {
        PRINT_WARNING("A font named '%s' already exists and will not be loaded again.", config->name);
        // Not a hard error, return success since it already exists and can be used.
        return true;
    }

    // Get a new id
    for (u16 i = 0; i < state_ptr->config.max_bitmap_font_count; ++i) {
        if (state_ptr->bitmap_fonts[i].id == INVALID_ID_U16) {
            id = i;
            break;
        }
    }
    if (id == INVALID_ID_U16) {
        PRINT_ERROR("No space left to allocate a new bitmap font. Increase maximum number allowed in font system config.");
        return false;
    }

    // Obtain the lookup.
    BITMAP_FONT_LOOKUP* lookup = &state_ptr->bitmap_fonts[id];

    if (!resource_system_load(config->resource_name, RESOURCE_TYPE_BITMAP_FONT, 0, &lookup->font.loaded_resource)) {
        PRINT_ERROR("Failed to load bitmap font.");
        return false;
    }

    // Keep a casted pointer to the resource data for convenience.
    lookup->font.resource_data = (BITMAP_FONT_RESOURCE_DATA*)lookup->font.loaded_resource.data;

    // Acquire the texture.
    // TODO: only accounts for one page at the moment.
    lookup->font.resource_data->data.atlas.texture = texture_system_acquire(lookup->font.resource_data->pages[0].file, true);

    b8 result = setup_font_data(&lookup->font.resource_data->data);

    // Set the entry id here last before updating the HASHTABLE.
    if (!hashtable_set(&state_ptr->bitmap_font_lookup, config->name, &id)) {
        PRINT_ERROR("Hashtable set failed on font load.");
        return false;
    }

    lookup->id = id;

    return result;
}

b8 font_system_acquire(const char* font_name, u16 font_size, struct UI_TEXT* text) {
    if (text->type == UI_TEXT_TYPE_BITMAP) {
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->bitmap_font_lookup, font_name, &id)) {
            PRINT_ERROR("Bitmap font lookup failed on acquire.");
            return false;
        }

        if (id == INVALID_ID_U16) {
            PRINT_ERROR("A bitmap font named '%s' was not found. Font acquisition failed.", font_name);
            return false;
        }

        // Get the lookup.
        BITMAP_FONT_LOOKUP* lookup = &state_ptr->bitmap_fonts[id];

        // Assign the data, increment the reference.
        text->data = &lookup->font.resource_data->data;
        lookup->reference_count++;

        return true;
    } else if (text->type == UI_TEXT_TYPE_SYSTEM) {
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->system_font_lookup, font_name, &id)) {
            PRINT_ERROR("System font lookup failed on acquire.");
            return false;
        }

        if (id == INVALID_ID_U16) {
            PRINT_ERROR("A system font named '%s' was not found. Font acquisition failed.", font_name);
            return false;
        }

        // Get the lookup.
        SYSTEM_FONT_LOOKUP* lookup = &state_ptr->system_fonts[id];

        // Search the size variants for the correct size.
        u32 count = darray_length(lookup->size_variants);
        for (u32 i = 0; i < count; ++i) {
            if (lookup->size_variants[i].size == font_size) {
                // Assign the data, increment the reference.
                text->data = &lookup->size_variants[i];
                lookup->reference_count++;
                return true;
            }
        }

        // If we reach this point, the size variant doesn't exist. Create it.
        FONT_DATA variant;
        if (!create_system_font_variant(lookup, font_size, font_name, &variant)) {
            PRINT_ERROR("Failed to create variant: %s, index %i, size %i", lookup->face, lookup->index, font_size);
            return false;
        }

        // Also perform setup for the variant
        if (!setup_font_data(&variant)) {
            PRINT_ERROR("Failed to setup font data");
        }

        // Add to the lookup's size variants.
        darray_push(lookup->size_variants, variant);
        u32 length = darray_length(lookup->size_variants);
        // Assign the data, increment the reference.
        text->data = &lookup->size_variants[length - 1];
        lookup->reference_count++;
        return true;
    }

    PRINT_ERROR("Unrecognized font type: %d", text->type);
    return false;
}

b8 font_system_release(struct UI_TEXT* text) {
    // TODO: Lookup font by name in appropriate HASHTABLE.
    return true;
}

b8 font_system_verify_atlas(FONT_DATA* font, const char* text) {
    if (font->type == FONT_TYPE_BITMAP) {
        // Bitmaps don't need verification since they are already generated.
        return true;
    } else if (font->type == FONT_TYPE_SYSTEM) {
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->system_font_lookup, font->face, &id)) {
            PRINT_ERROR("System font lookup failed on acquire.");
            return false;
        }

        if (id == INVALID_ID_U16) {
            PRINT_ERROR("A system font named '%s' was not found. Font atlas verification failed.", font->face);
            return false;
        }

        // Get the lookup.
        SYSTEM_FONT_LOOKUP* lookup = &state_ptr->system_fonts[id];

        return verify_system_font_size_variant(lookup, font, text);
    }

    PRINT_ERROR("font_system_verify_atlas failed: Unknown font type.");
    return false;
}

b8 setup_font_data(FONT_DATA* font) {
    // Create map resources
    font->atlas.filter_magnify = font->atlas.filter_minify = TEXTURE_FILTER_MODE_LINEAR;
    font->atlas.repeat_u = font->atlas.repeat_v = font->atlas.repeat_w = TEXTURE_REPEAT_CLAMP_TO_EDGE;
    font->atlas.use = TEXTURE_USE_MAP_DIFFUSE;
    if (!renderer_texture_map_acquire_resources(&font->atlas)) {
        PRINT_ERROR("Unable to acquire resources for font atlas texture map.");
        return false;
    }

    // Check for a tab glyph, as there may not always be one exported. If there is, store its
    // x_advance and just use that. If there is not, then create one based off spacex4
    if (!font->tab_x_advance) {
        for (u32 i = 0; i < font->glyph_count; ++i) {
            if (font->glyphs[i].codepoint == '\t') {
                font->tab_x_advance = font->glyphs[i].x_advance;
                break;
            }
        }
        // If still not found, use space x 4.
        if (!font->tab_x_advance) {
            for (u32 i = 0; i < font->glyph_count; ++i) {
                // Search for space
                if (font->glyphs[i].codepoint == ' ') {
                    font->tab_x_advance = font->glyphs[i].x_advance * 4;
                    break;
                }
            }
            if (!font->tab_x_advance) {
                // If _still_ not there, then a space wasn't present either, so just
                // hardcode something, in this case font size * 4.
                font->tab_x_advance = font->size * 4;
            }
        }
    }

    return true;
}

void cleanup_font_data(FONT_DATA* font) {
    // Release the texture map resources.
    renderer_texture_map_release_resources(&font->atlas);

    // If a bitmap font, release the reference to the texture.
    if (font->type == FONT_TYPE_BITMAP && font->atlas.texture) {
        texture_system_release(font->atlas.texture->name);
    }
    font->atlas.texture = 0;
}

b8 create_system_font_variant(SYSTEM_FONT_LOOKUP* lookup, u16 size, const char* font_name, FONT_DATA* out_variant) {
    yzero_memory(out_variant, sizeof(FONT_DATA));
    out_variant->atlas_size_x = 1024;  // TODO: configurable size
    out_variant->atlas_size_y = 1024;
    out_variant->size = size;
    out_variant->type = FONT_TYPE_SYSTEM;
    string_ncopy(out_variant->face, font_name, 255);
    out_variant->internal_data_size = sizeof(SYSTEM_FONT_VARIANT_DATA);
    out_variant->internal_data = yallocate_aligned(out_variant->internal_data_size, 8, MEMORY_TAG_SYSTEM_FONT);

    SYSTEM_FONT_VARIANT_DATA* internal_data = (SYSTEM_FONT_VARIANT_DATA*)out_variant->internal_data;

    // Push default codepoints (ascii 32-127) always, plus a -1 for unknown.
    internal_data->codepoints = darray_reserve(i32, 96);
    darray_push(internal_data->codepoints, -1);  // push invalid char
    for (i32 i = 0; i < 95; ++i) {
        internal_data->codepoints[i + 1] = i + 32;
    }
    darray_length_set(internal_data->codepoints, 96);

    // Create texture.
    char font_tex_name[255];
    string_format(font_tex_name, "__system_text_atlas_%s_i%i_sz%i__", font_name, lookup->index, size);
    out_variant->atlas.texture = texture_system_aquire_writeable(font_tex_name, out_variant->atlas_size_x, out_variant->atlas_size_y, 4, true);

    // Obtain some metrics
    internal_data->scale = stbtt_ScaleForPixelHeight(&lookup->info, (f32)size);
    i32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&lookup->info, &ascent, &descent, &line_gap);
    out_variant->line_height = (ascent - descent + line_gap) * internal_data->scale;

    return rebuild_system_font_variant_atlas(lookup, out_variant);
}

b8 rebuild_system_font_variant_atlas(SYSTEM_FONT_LOOKUP* lookup, FONT_DATA* variant) {
    SYSTEM_FONT_VARIANT_DATA* internal_data = (SYSTEM_FONT_VARIANT_DATA*)variant->internal_data;

    u32 pack_image_size = variant->atlas_size_x * variant->atlas_size_y * sizeof(u8);
    u8* pixels = yallocate(pack_image_size, MEMORY_TAG_ARRAY);
    u32 codepoint_count = darray_length(internal_data->codepoints);
    stbtt_packedchar* packed_chars = yallocate(sizeof(stbtt_packedchar) * codepoint_count, MEMORY_TAG_ARRAY);

    // Begin packing all known characters into the atlas. This
    // creates a single-channel image with rendered glyphs at the
    // given size.
    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, pixels, variant->atlas_size_x, variant->atlas_size_y, 0, 1, 0)) {
        PRINT_ERROR("stbtt_PackBegin failed");
        return false;
    }

    // Fit all codepoints into a single range for packing.
    stbtt_pack_range range;
    range.first_unicode_codepoint_in_range = 0;
    range.font_size = variant->size;
    range.num_chars = codepoint_count;
    range.chardata_for_range = packed_chars;
    range.array_of_unicode_codepoints = internal_data->codepoints;
    if (!stbtt_PackFontRanges(&context, lookup->font_binary, lookup->index, &range, 1)) {
        PRINT_ERROR("stbtt_PackFontRanges failed");
        return false;
    }

    stbtt_PackEnd(&context);
    // Packing complete.

    // Convert from single-channel to RGBA, or pack_image_size * 4.
    u8* rgba_pixels = yallocate(pack_image_size * 4, MEMORY_TAG_ARRAY);
    for (u32 j = 0; j < pack_image_size; ++j) {
        rgba_pixels[(j * 4) + 0] = pixels[j];
        rgba_pixels[(j * 4) + 1] = pixels[j];
        rgba_pixels[(j * 4) + 2] = pixels[j];
        rgba_pixels[(j * 4) + 3] = pixels[j];
    }

    // Write texture data to atlas.
    texture_system_write_data(variant->atlas.texture, 0, pack_image_size * 4, rgba_pixels);

    // Free pixel/rgba_pixel data.
    yfree(pixels);
    yfree(rgba_pixels);

    // Regenerate glyphs
    if (variant->glyphs && variant->glyph_count) {
        yfree(variant->glyphs);
    }
    variant->glyph_count = codepoint_count;
    variant->glyphs = yallocate(sizeof(FONT_GLYPH) * codepoint_count, MEMORY_TAG_ARRAY);
    for (u16 i = 0; i < variant->glyph_count; ++i) {
        stbtt_packedchar* pc = &packed_chars[i];
        FONT_GLYPH* g = &variant->glyphs[i];
        g->codepoint = internal_data->codepoints[i];
        g->page_id = 0;
        g->x_offset = pc->xoff;
        g->y_offset = pc->yoff;
        g->x = pc->x0;  // xmin;
        g->y = pc->y0;
        g->width = pc->x1 - pc->x0;
        g->height = pc->y1 - pc->y0;
        g->x_advance = pc->xadvance;
    }

    // Regenerate kernings
    if (variant->kernings && variant->kerning_count) {
        yfree(variant->kernings);
    }
    variant->kerning_count = stbtt_GetKerningTableLength(&lookup->info);
    if (variant->kerning_count) {
        variant->kernings = yallocate(sizeof(FONT_KERNING) * variant->kerning_count, MEMORY_TAG_ARRAY);
        // Get the kerning table for the current font.
        stbtt_kerningentry* kerning_table = yallocate(sizeof(stbtt_kerningentry) * variant->kerning_count, MEMORY_TAG_ARRAY);
        u32 entry_count = stbtt_GetKerningTable(&lookup->info, kerning_table, variant->kerning_count);
        if (entry_count != variant->kerning_count) {
            PRINT_ERROR("Kerning entry count mismatch: %u->%u", entry_count, variant->kerning_count);
            return false;
        }

        for (u32 i = 0; i < variant->kerning_count; ++i) {
            FONT_KERNING* k = &variant->kernings[i];
            k->codepoint_0 = kerning_table[i].glyph1;
            k->codepoint_1 = kerning_table[i].glyph2;
            k->amount = kerning_table[i].advance;
        }
    } else {
        variant->kernings = 0;
    }

    return true;
}

b8 verify_system_font_size_variant(SYSTEM_FONT_LOOKUP* lookup, FONT_DATA* variant, const char* text) {
    SYSTEM_FONT_VARIANT_DATA* internal_data = (SYSTEM_FONT_VARIANT_DATA*)variant->internal_data;

    u32 char_length = string_length(text);
    u32 added_codepoint_count = 0;
    for (u32 i = 0; i < char_length;) {
        i32 codepoint;
        u8 advance;
        if (!bytes_to_codepoint(text, i, &codepoint, &advance)) {
            PRINT_ERROR("bytes_to_codepoint failed to get codepoint.");
            ++i;
            continue;
        } else {
            // Check if the codepoint is already contained. Note that ascii
            // codepoints are always included, so checking those may be skipped.
            i += advance;
            if (codepoint < 128) {
                continue;
            }
            u32 codepoint_count = darray_length(internal_data->codepoints);
            b8 found = false;
            for (u32 j = 95; j < codepoint_count; ++j) {
                if (internal_data->codepoints[j] == codepoint) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                darray_push(internal_data->codepoints, codepoint);
                added_codepoint_count++;
            }
        }
    }

    // If codepoints were added, rebuild the atlas.
    if (added_codepoint_count > 0) {
        return rebuild_system_font_variant_atlas(lookup, variant);
    }

    // Otherwise, proceed as normal.
    return true;
}
