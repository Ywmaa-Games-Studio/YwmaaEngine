#pragma once

#include "math/math_types.h"
#include "resources/resource_types.h"
#include "renderer/renderer_types.inl"

struct FONT_DATA;

typedef enum E_UI_TEXT_TYPE {
    UI_TEXT_TYPE_BITMAP,
    UI_TEXT_TYPE_SYSTEM
} E_UI_TEXT_TYPE;

/* typedef enum E_TEXT_DIRECTION {
	TEXT_DIRECTION_LEFT_TO_RIGHT,
	TEXT_DIRECTION_RIGHT_TO_LEFT,
	TEXT_DIRECTION_TOP_TO_BOTTOM,
	TEXT_DIRECTION_BOTTOM_TO_TOP,
	TEXT_DIRECTION_ANY,
} E_TEXT_DIRECTION; */

typedef struct UI_TEXT {
    u32 unique_id;
    E_UI_TEXT_TYPE type;
    //E_TEXT_DIRECTION direction;
    struct FONT_DATA* data;
    RENDER_BUFFER vertex_buffer;
    RENDER_BUFFER index_buffer;
    char* text;
    Transform transform;
    u32 instance_id;
    u64 render_frame_number;
} UI_TEXT;

YAPI b8 ui_text_create(E_UI_TEXT_TYPE type, const char* font_name, u16 font_size, const char* text_content, UI_TEXT* out_text);
YAPI void ui_text_destroy(UI_TEXT* text);

/**
 * @brief Sets the position on the given UI text object.
 *
 * @param u_text A pointer to the UI text whose text will be set.
 * @param text The position to be set.
 */
YAPI void ui_text_position_set(UI_TEXT* u_text, Vector3 position);
/**
 * @brief Sets the text on the given UI text object.
 *
 * @param u_text A pointer to the UI text whose text will be set.
 * @param text The text to be set.
 */
YAPI void ui_text_text_set(UI_TEXT* u_text, const char* text);
/**
 * @brief Draws the given UI text.
 *
 * @param u_text A pointer to the text to be drawn.
 */
YAPI void ui_text_draw(UI_TEXT* u_text);
