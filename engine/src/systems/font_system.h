/**
 * @file font_system.h=
 * @brief A system responsible for the management of bitmap
 * and system fonts.
 * 
 */
#pragma once

#include "math/math_types.h"
#include "renderer/renderer_types.inl"

/**
 * @brief The configuration for a system font.
 */
typedef struct SYSTEM_FONT_CONFIG {
    /** @brief The name of the font. */
    char* name;
    /** @brief The default size of the font. */
    u16 default_size;
    /** @brief The name of the resource containing the font data. */
    char* resource_name;
} SYSTEM_FONT_CONFIG;

/**
 * @brief The configuration for a bitmap font.
 */
typedef struct BITMAP_FONT_CONFIG {
    /** @brief The name of the font. */
    char* name;
    /** @brief The size of the font. */
    u16 size;
    /** @brief The name of the resource containing the font data. */
    char* resource_name;
} BITMAP_FONT_CONFIG;

/**
 * @brief The configuration of the font system.
 * Should be setup by the application during the boot
 * process.
 */
typedef struct FONT_SYSTEM_CONFIG {
    /** @brief The default number of system fonts. */
    u8 default_system_font_count;
    /** @brief The default system font configs. */
    SYSTEM_FONT_CONFIG* system_font_configs;
    /** @brief The default number of bitmap fonts. */
    u8 default_bitmap_font_count;
    /** @brief The default bitmap font configs. */
    BITMAP_FONT_CONFIG* bitmap_font_configs;
    /** @brief The default number of system fonts. */
    u8 max_system_font_count;
    /** @brief The default number of bitmap fonts. */
    u8 max_bitmap_font_count;
    /** @brief Indicates if fonts should auto-release when not used. */
    b8 auto_release;
} FONT_SYSTEM_CONFIG;

struct UI_TEXT;

/**
 * @brief Initializes the font system. As with other systems, this should
 * be called twice; once to get the memory requirement (where memory = 0),
 * and a second time passing allocated memory.
 * 
 * @param memory_requirement A pointer to hold the memory requirement.
 * @param memory The allocated memory for the system state.
 * @param config The font system config.
 * @return True on success; otherwise false.
 */
b8 font_system_init(u64* memory_requirement, void* memory, void* config);
/**
 * @brief Shuts down the font system.
 * 
 * @param memory The system state memory.
 */
void font_system_shutdown(void* memory);

/**
 * @brief Loads a system font from the following config.
 * 
 * @param config A pointer to the config to use for loading.
 * @return True on success; otherwise false.
 */
b8 font_system_load_system_font(SYSTEM_FONT_CONFIG* config);
/**
 * @brief Loads a bitmap font from the following config.
 * 
 * @param config A pointer to the config to use for loading.
 * @return True on success; otherwise false.
 */
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

/**
 * @brief Verifies the atlas of the provided font contains
 * the characters in text.
 * 
 * @param font A pointer to the font to be verified.
 * @param text The text containing the characters required.
 * @return True on success; otherwise false.
 */
b8 font_system_verify_atlas(FONT_DATA* font, const char* text);
