#pragma once

#include "math/math_types.h"

#define TEXTURE_NAME_MAX_LENGTH 512

typedef struct TEXTURE {
    u32 id;
    u32 width;
    u32 height;
    u8 channel_count;
    b8 has_transparency;
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* internal_data;
} TEXTURE;

typedef enum E_TEXTURE_USE {
    TEXTURE_USE_UNKNOWN = 0x00,
    TEXTURE_USE_MAP_DIFFUSE = 0x01
} E_TEXTURE_USE;

typedef struct TEXTURE_MAP {
    TEXTURE* texture;
    E_TEXTURE_USE use;
} TEXTURE_MAP;

#define MATERIAL_NAME_MAX_LENGTH 256
typedef struct MATERIAL {
    u32 id;
    u32 generation;
    u32 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vector4 diffuse_colour;
    TEXTURE_MAP diffuse_map;
} MATERIAL;


#define GEOMETRY_NAME_MAX_LENGTH 256

/**
 * @brief Represents actual geometry in the world.
 * Typically (but not always, depending on use) paired with a material.
 */
typedef struct GEOMETRY {
    u32 id;
    u32 internal_id;
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    MATERIAL* material;
} GEOMETRY;