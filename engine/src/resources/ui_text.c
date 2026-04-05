#include "ui_text.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include "core/identifier.h"
#include "math/ymath.h"
#include "math/transform.h"
#include "renderer/renderer_types.inl"
#include "renderer/renderer_frontend.h"

#include "systems/font_system.h"
#include "systems/shader_system.h"

#include "arabic_utils.h"

static void regenerate_geometry(UI_TEXT* text);

b8 ui_text_create(E_UI_TEXT_TYPE type, const char* font_name, u16 font_size, const char* text_content, UI_TEXT* out_text) {
    if (!font_name || !text_content || !out_text) {
        PRINT_ERROR("ui_text_create requires a valid pointer to font_name, text_content and out_text");
        return false;
    }

    yzero_memory(out_text, sizeof(UI_TEXT));

    // Assign the type first
    out_text->type = type;

    // Acquire the font of the correct type and assign its internal data.
    // This also gets the atlas texture.
    if (!font_system_acquire(font_name, font_size, out_text)) {
        PRINT_ERROR("Unable to acquire font: '%s'. UI_TEXT cannot be created.", font_name);
        return false;
    }

    out_text->text = string_duplicate(text_content);
    out_text->transform = transform_create();

    out_text->instance_id = INVALID_ID;
    out_text->render_frame_number = INVALID_ID_U64;

    static const u64 quad_size = (sizeof(Vertex2D) * 4);

    u32 text_length = string_length(out_text->text);
    // In the case of an empty string, cannot create an empty buffer so just create enough to hold one for now.
    if (text_length < 1) {
        text_length = 1;
    }

    // Acquire resources for font texture map.
    SHADER* ui_shader = shader_system_get("shader.builtin.ui");  // TODO: text SHADER.
    TEXTURE_MAP* font_maps[1] = {&out_text->data->atlas};
    if (!renderer_shader_instance_resources_acquire(ui_shader, font_maps, &out_text->instance_id)) {
        PRINT_ERROR("Unable to acquire SHADER resources for font texture map.");
        return false;
    }

    // Generate the vertex buffer.
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_VERTEX, text_length * quad_size, false, &out_text->vertex_buffer)) {
        PRINT_ERROR("ui_text_create failed to create vertex renderbuffer.");
        return false;
    }
    if (!renderer_renderbuffer_bind(&out_text->vertex_buffer, 0)) {
        PRINT_ERROR("ui_text_create failed to bind vertex renderbuffer.");
        return false;
    }

    // Generate an index buffer.
    static const u8 quad_index_size = sizeof(u32) * 6;
    if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_INDEX, text_length * quad_index_size, false, &out_text->index_buffer)) {
        PRINT_ERROR("ui_text_create failed to create index renderbuffer.");
        return false;
    }
    if (!renderer_renderbuffer_bind(&out_text->index_buffer, 0)) {
        PRINT_ERROR("ui_text_create failed to bind index renderbuffer.");
        return false;
    }

    // Verify atlas has the glyphs needed.
    if (!font_system_verify_atlas(out_text->data, text_content)) {
        PRINT_ERROR("Font atlas verification failed.");
        return false;
    }

    // Generate geometry.
    regenerate_geometry(out_text);

    // Get a unique identifier for the text object.
    out_text->unique_id = identifier_aquire_new_id(out_text);

    return true;
}

void ui_text_destroy(UI_TEXT* text) {
    if (text) {
        // Release the unique identifier.
        identifier_release_id(text->unique_id);

        if (text->text) {
            yfree(text->text);
            text->text = 0;
        }

        // Destroy buffers.
        renderer_renderbuffer_destroy(&text->vertex_buffer);
        renderer_renderbuffer_destroy(&text->index_buffer);

        // Release resources for font texture map.
        SHADER* ui_shader = shader_system_get("shader.builtin.ui");  // TODO: text SHADER.
        if (!renderer_shader_instance_resources_release(ui_shader, text->instance_id)) {
            PRINT_ERROR("Unable to release SHADER resources for font texture map.");
        }
    }
    yzero_memory(text, sizeof(UI_TEXT));
}

void ui_text_position_set(UI_TEXT* u_text, Vector3 position) {
    transform_position_set(&u_text->transform, position);
}

void ui_text_text_set(UI_TEXT* u_text, const char* text) {
    if (u_text && u_text->text) {
        // If strings are already equal, don't do anything.
        if (strings_equal(text, u_text->text)) {
            return;
        }

        yfree(u_text->text);
        u_text->text = string_duplicate(text);

        // Verify atlas has the glyphs needed.
        if (!font_system_verify_atlas(u_text->data, text)) {
            PRINT_ERROR("Font atlas verification failed.");
        }

        regenerate_geometry(u_text);
    }
}

void ui_text_draw(UI_TEXT* u_text) {
    // TODO: utf8 length
    u32 text_length = string_length(u_text->text);
    if (text_length > 0) {
        static const u64 quad_vert_count = 4;
        if (!renderer_renderbuffer_draw(&u_text->vertex_buffer, 0, text_length * quad_vert_count, true)) {
            PRINT_ERROR("Failed to draw ui font vertex buffer.");
        }

        static const u8 quad_index_count = 6;
        if (!renderer_renderbuffer_draw(&u_text->index_buffer, 0, text_length * quad_index_count, false)) {
            PRINT_ERROR("Failed to draw ui font index buffer.");
        }
    }
}
// for debugging
/* i32 utf8_encode(u32 codepoint, char *out) {
    if (codepoint <= 0x7F) {
        out[0] = codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        out[0] = 0xC0 | (codepoint >> 6);
        out[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint <= 0xFFFF) {
        out[0] = 0xE0 | (codepoint >> 12);
        out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        out[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        out[0] = 0xF0 | (codepoint >> 18);
        out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        out[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    }
    return 0; // Invalid code point
} */

static void regenerate_geometry(UI_TEXT* text) {
    // Get the UTF-8 string length
    u32 text_length_utf8 = string_utf8_length(text->text);
    // Also get the length in characters.
    u32 char_length = string_length(text->text);

    // Don't try to regenerate geometry for something that doesn't have any text.
    if (text_length_utf8 < 1) {
        return;
    }

    // Calculate buffer sizes.
    static const u64 verts_per_quad = 4;
    static const u8 indices_per_quad = 6;
    u64 vertex_buffer_size = sizeof(Vertex2D) * verts_per_quad * text_length_utf8;
    u64 index_buffer_size = sizeof(u32) * indices_per_quad * text_length_utf8;

    // Resize the vertex buffer, but only if larger.
    if (vertex_buffer_size > text->vertex_buffer.total_size) {
        if (!renderer_renderbuffer_resize(&text->vertex_buffer, vertex_buffer_size)) {
            PRINT_ERROR("regenerate_geometry for ui text failed to resize vertex renderbuffer.");
            return;
        }
    }

    // Resize the index buffer, but only if larger.
    if (index_buffer_size > text->index_buffer.total_size) {
        if (!renderer_renderbuffer_resize(&text->index_buffer, index_buffer_size)) {
            PRINT_ERROR("regenerate_geometry for ui text failed to resize index renderbuffer.");
            return;
        }
    }
    i32 codepoint_prev = 0;  // Previous codepoint
    u32 text_width = 0;
    for (u32 c = 0; c < char_length; ++c) {
        i32 codepoint = text->text[c];

        // Continue to next line for newline.
        if (codepoint == '\n') {
            //x = 0;
            continue;
        }

        if (codepoint == '\t') {
            text_width += text->data->tab_x_advance;
            continue;
        }

        // NOTE: UTF-8 codepoint handling.
        u8 advance = 0;
        if (!bytes_to_codepoint(text->text, c, &codepoint, &advance)) {
            PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
            codepoint = -1;
        }

        b8 is_arabic = is_character_in_arabic_codepoint_range(codepoint);
        b8 is_arabic_diacritics = is_diacritics_or_sign(codepoint);
        // Get the offset of the next character. If there is no advance, move forward one,
        // otherwise use advance as-is.
        if (is_arabic) {
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            // Get the next codepoint.
            i32 next_codepoint = 0;
            u8 advance_next = 0;
            if (!bytes_to_codepoint(text->text, offset, &next_codepoint, &advance_next)) {
                PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
                next_codepoint = -1;
            } else {
                while (is_diacritics_or_sign(next_codepoint))
                {
                    offset += advance_next;
                    if (!bytes_to_codepoint(text->text, offset, &next_codepoint, &advance_next)){
                        PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
                        next_codepoint = -1;
                    }
                }
                u32 arabic_codepoint = get_presentation_form_for_char(codepoint_prev, next_codepoint, codepoint);
                codepoint_prev = codepoint;
                codepoint = arabic_codepoint;
                if (arabic_codepoint == -1){
                    // Now advance c
                    c += advance - 1;  // Subtracting 1 because the loop always increments once for single-byte anyway.
                    continue;
                }
            }
        } else {
            if (!is_arabic_diacritics) {
                codepoint_prev = codepoint;
            }
        }

        FONT_GLYPH* g = 0;
        for (u32 i = 0; i < text->data->glyph_count; ++i) {
            if (text->data->glyphs[i].codepoint == codepoint) {
                g = &text->data->glyphs[i];
                break;
            }
        }

        if (!g) {
            // If not found, use the codepoint -1
            codepoint = -1;
            for (u32 i = 0; i < text->data->glyph_count; ++i) {
                if (text->data->glyphs[i].codepoint == codepoint) {
                    g = &text->data->glyphs[i];
                    break;
                }
            }
        }

        if (g) {
            if (!is_arabic_diacritics) {
                text_width += g->x_advance;
            }
        } else {
            PRINT_ERROR("Unable to find unknown codepoint. Skipping.");
            continue;
        }

        // Now advance c
        c += advance - 1;  // Subtracting 1 because the loop always increments once for single-byte anyway.
    }

    // Generate new geometry for each character.
    f32 x = 0;
    f32 y = 0;
    // Temp arrays to hold vertex/index data.
    Vertex2D* vertex_buffer_data = yallocate(vertex_buffer_size, MEMORY_TAG_ARRAY);
    u32* index_buffer_data = yallocate(index_buffer_size, MEMORY_TAG_ARRAY);

    codepoint_prev = 0;  // Previous codepoint
    // Take the length in chars and get the correct codepoint from it.
    for (u32 c = 0, uc = 0; c < char_length; ++c) {
        i32 codepoint = text->text[c];

        // Continue to next line for newline.
        if (codepoint == '\n') {
            x = 0;
            y += text->data->line_height;
            // Increment utf-8 character count.
            uc++;
            continue;
        }

        if (codepoint == '\t') {
            x += text->data->tab_x_advance;
            uc++;
            continue;
        }

        // NOTE: UTF-8 codepoint handling.
        u8 advance = 0;
        if (!bytes_to_codepoint(text->text, c, &codepoint, &advance)) {
            PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
            codepoint = -1;
        }
        b8 is_arabic = is_character_in_arabic_codepoint_range(codepoint);
        b8 is_arabic_diacritics = is_diacritics_or_sign(codepoint);
        // Get the offset of the next character. If there is no advance, move forward one,
        // otherwise use advance as-is.
        if (is_arabic) {
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            // Get the next codepoint.
            i32 next_codepoint = 0;
            u8 advance_next = 0;
            if (!bytes_to_codepoint(text->text, offset, &next_codepoint, &advance_next)) {
                PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
                next_codepoint = -1;
            } else {
                while (is_diacritics_or_sign(next_codepoint))
                {
                    offset += advance_next;
                    if (!bytes_to_codepoint(text->text, offset, &next_codepoint, &advance_next)){
                        PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
                        next_codepoint = -1;
                    }
                }
                u32 arabic_codepoint = get_presentation_form_for_char(codepoint_prev, next_codepoint, codepoint);
                codepoint_prev = codepoint;
                codepoint = arabic_codepoint;

                if (arabic_codepoint == -1){
                    // Now advance c
                    c += advance - 1;  // Subtracting 1 because the loop always increments once for single-byte anyway.
                    // Increment utf-8 character count.
                    uc++;
                    continue;
                }
            }
/*             char utf8_encode_result[4];
            utf8_encode_result[3] = '\0';
            utf8_encode(codepoint, utf8_encode_result);
            PRINT_DEBUG("arabic codepoint %i utf8 result: %s arabic? %i character index %i full utf8 length %i", codepoint, utf8_encode_result, is_arabic, uc, text_length_utf8); */
        } else {
            if (!is_arabic_diacritics) {
                codepoint_prev = codepoint;
            }
        }

        FONT_GLYPH* g = 0;
        for (u32 i = 0; i < text->data->glyph_count; ++i) {
            if (text->data->glyphs[i].codepoint == codepoint) {
                g = &text->data->glyphs[i];
                break;
            }
        }

        if (!g) {
            // If not found, use the codepoint -1
            codepoint = -1;
            for (u32 i = 0; i < text->data->glyph_count; ++i) {
                if (text->data->glyphs[i].codepoint == codepoint) {
                    g = &text->data->glyphs[i];
                    break;
                }
            }
        }

        if (g) {
            // Found the glyph. generate points.
            f32 minx = x + g->x_offset;
            f32 miny = y + g->y_offset;
            f32 maxx = minx + g->width;
            f32 maxy = miny + g->height;
            f32 tminx = (f32)g->x / text->data->atlas_size_x;
            f32 tmaxx = (f32)(g->x + g->width) / text->data->atlas_size_x;
            f32 tminy = (f32)g->y / text->data->atlas_size_y;
            f32 tmaxy = (f32)(g->y + g->height) / text->data->atlas_size_y;

            if (is_arabic || is_arabic_diacritics) {
                if (is_arabic_diacritics) {
                    x -= g->x_advance;
                }
                // For Arabic, we need to flip the x-axis.
                minx = (text_width - x) + g->x_offset;
                maxx = minx + g->width;
                if (is_arabic_diacritics) {
                    x += g->x_advance;
                }
            }
            // Flip the y axis for system text
            if (text->type == UI_TEXT_TYPE_SYSTEM) {
                tminy = 1.0f - tminy;
                tmaxy = 1.0f - tmaxy;
            }

            Vertex2D p0 = (Vertex2D){Vector2_create(minx, miny), Vector2_create(tminx, tminy)};
            Vertex2D p1 = (Vertex2D){Vector2_create(maxx, miny), Vector2_create(tmaxx, tminy)};
            Vertex2D p2 = (Vertex2D){Vector2_create(maxx, maxy), Vector2_create(tmaxx, tmaxy)};
            Vertex2D p3 = (Vertex2D){Vector2_create(minx, maxy), Vector2_create(tminx, tmaxy)};

            vertex_buffer_data[(uc * 4) + 0] = p0;  // 0    3
            vertex_buffer_data[(uc * 4) + 1] = p2;  //
            vertex_buffer_data[(uc * 4) + 2] = p3;  //
            vertex_buffer_data[(uc * 4) + 3] = p1;  // 2    1

            // Try to find kerning
            i32 kerning = 0;

            // Get the offset of the next character. If there is no advance, move forward one,
            // otherwise use advance as-is.
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            if (offset < text_length_utf8 - 1) {
                // Get the next codepoint.
                i32 next_codepoint = 0;
                u8 advance_next = 0;

                if (!bytes_to_codepoint(text->text, offset, &next_codepoint, &advance_next)) {
                    PRINT_WARNING("Invalid UTF-8 found in string, using unknown codepoint of -1");
                    codepoint = -1;
                } else {
                    for (u32 i = 0; i < text->data->kerning_count; ++i) {
                        FONT_KERNING* k = &text->data->kernings[i];
                        if (k->codepoint_0 == codepoint && k->codepoint_1 == next_codepoint) {
                            kerning = k->amount;
                        }
                    }
                }
            }
            x += g->x_advance + kerning;

        } else {
            PRINT_ERROR("Unable to find unknown codepoint. Skipping.");
            // Increment utf-8 character count.
            uc++;
            continue;
        }

        // Index data 210301
        index_buffer_data[(uc * 6) + 0] = (uc * 4) + 2;
        index_buffer_data[(uc * 6) + 1] = (uc * 4) + 1;
        index_buffer_data[(uc * 6) + 2] = (uc * 4) + 0;
        index_buffer_data[(uc * 6) + 3] = (uc * 4) + 3;
        index_buffer_data[(uc * 6) + 4] = (uc * 4) + 0;
        index_buffer_data[(uc * 6) + 5] = (uc * 4) + 1;

        // Now advance c
        c += advance - 1;  // Subtracting 1 because the loop always increments once for single-byte anyway.
        // Increment utf-8 character count.
        uc++;
    }

    // Load up the data.
    b8 vertex_load_result = renderer_renderbuffer_load_range(&text->vertex_buffer, 0, vertex_buffer_size, vertex_buffer_data);
    b8 index_load_result = renderer_renderbuffer_load_range(&text->index_buffer, 0, index_buffer_size, index_buffer_data);

    // Clean up.
    yfree(vertex_buffer_data);
    yfree(index_buffer_data);

    // Verify results.
    if (!vertex_load_result) {
        PRINT_ERROR("regenerate_geometry failed to load data into vertex buffer range.");
    }
    if (!index_load_result) {
        PRINT_ERROR("regenerate_geometry failed to load data into index buffer range.");
    }
}
