#pragma once

#include "math/math_types.h"
#include "renderer/renderer_types.inl"

typedef struct SYSTEM_FONT_CONFIG {
    char* name;
    u16 default_size;
    char* resource_name;
} SYSTEM_FONT_CONFIG;

typedef struct BITMAP_FONT_CONFIG {
    char* name;
    u16 size;
    char* resource_name;
} BITMAP_FONT_CONFIG;

typedef struct FONT_SYSTEM_CONFIG {
    u8 default_system_font_count;
    SYSTEM_FONT_CONFIG* system_font_configs;
    u8 default_bitmap_font_count;
    BITMAP_FONT_CONFIG* bitmap_font_configs;
    u8 max_system_font_count;
    u8 max_bitmap_font_count;
    b8 auto_release;
} FONT_SYSTEM_CONFIG;

struct UI_TEXT;

b8 font_system_init(u64* memory_requirement, void* memory, FONT_SYSTEM_CONFIG* config);
void font_system_shutdown(void* memory);

b8 font_system_load_system_font(SYSTEM_FONT_CONFIG* config);
b8 font_system_load_bitmap_font(BITMAP_FONT_CONFIG* config);

/**
 * @brief Attempts to acquire a font of the given name and assign it to the given UI_TEXT.
 *
 * @param font_name The name of the font to acquire. Must be an already loaded font.
 * @param font_size The font size. Ignored for bitmap fonts.
 * @param text A pointer to the text object for which to acquire the font.
 * @return True on success; otherwise false.
 */
b8 font_system_acquire(const char* font_name, u16 font_size, struct UI_TEXT* text);

/**
 * @brief Releases references to the font held by the provided UI_TEXT.
 *
 * @param text A pointer to the text object to release the font from.
 * @return True on success; otherwise false.
 */
b8 font_system_release(struct UI_TEXT* text);

b8 font_system_verify_atlas(FONT_DATA* font, const char* text);
