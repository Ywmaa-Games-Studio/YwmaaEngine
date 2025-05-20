#pragma once

#include "math/math_types.h"

// Pre-defined resource types.
typedef enum E_RESOURCE_TYPE {
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_STATIC_MESH,
    RESOURCE_TYPE_CUSTOM
} E_RESOURCE_TYPE;

typedef struct RESOURCE {
    u32 loader_id;
    const char* name;
    char* full_path;
    u64 data_size;
    void* data;
} RESOURCE;

typedef struct IMAGE_RESOURCE_DATA {
    u8 channel_count;
    u32 width;
    u32 height;
    u8* pixels;
} IMAGE_RESOURCE_DATA;


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
typedef enum E_MATERIAL_TYPE {
    MATERIAL_TYPE_WORLD,
    MATERIAL_TYPE_UI
} E_MATERIAL_TYPE;
typedef struct MATERIAL_CONFIG {
    char name[MATERIAL_NAME_MAX_LENGTH];
    E_MATERIAL_TYPE type;
    b8 auto_release;
    Vector4 diffuse_colour;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} MATERIAL_CONFIG;

typedef struct MATERIAL {
    u32 id;
    u32 generation;
    u32 internal_id;
    E_MATERIAL_TYPE type;
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
