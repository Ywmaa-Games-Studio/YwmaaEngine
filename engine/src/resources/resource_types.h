#pragma once

#include "math/math_types.h"

// Pre-defined resource types.
typedef enum E_RESOURCE_TYPE {
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_STATIC_MESH,
    RESOURCE_TYPE_SHADER,
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
typedef struct MATERIAL_CONFIG {
    char name[MATERIAL_NAME_MAX_LENGTH];
    char* shader_name;
    b8 auto_release;
    Vector4 diffuse_colour;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} MATERIAL_CONFIG;

typedef struct MATERIAL {
    u32 id;
    u32 generation;
    u32 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vector4 diffuse_colour;
    TEXTURE_MAP diffuse_map;

    u32 shader_id;
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


/** @brief Shader stages available in the system. */
typedef enum E_SHADER_STAGE {
    SHADER_STAGE_VERTEX = 0x00000001,
    SHADER_STAGE_GEOMETRY = 0x00000008,
    SHADER_STAGE_FRAGMENT = 0x00000010,
    SHADER_STAGE_COMPUTE = 0x00000020
} E_SHADER_STAGE;

/** @brief Available attribute types. */
typedef enum E_SHADER_ATTRIBUTE_TYPE {
    SHADER_ATTRIB_TYPE_FLOAT32 = 0U,
    SHADER_ATTRIB_TYPE_FLOAT32_2 = 1U,
    SHADER_ATTRIB_TYPE_FLOAT32_3 = 2U,
    SHADER_ATTRIB_TYPE_FLOAT32_4 = 3U,
    SHADER_ATTRIB_TYPE_MATRIX_4 = 4U,
    SHADER_ATTRIB_TYPE_INT8 = 5U,
    SHADER_ATTRIB_TYPE_UINT8 = 6U,
    SHADER_ATTRIB_TYPE_INT16 = 7U,
    SHADER_ATTRIB_TYPE_UINT16 = 8U,
    SHADER_ATTRIB_TYPE_INT32 = 9U,
    SHADER_ATTRIB_TYPE_UINT32 = 10U,
} E_SHADER_ATTRIBUTE_TYPE;

/** @brief Available uniform types. */
typedef enum E_SHADER_UNIFORM_TYPE {
    SHADER_UNIFORM_TYPE_FLOAT32 = 0U,
    SHADER_UNIFORM_TYPE_FLOAT32_2 = 1U,
    SHADER_UNIFORM_TYPE_FLOAT32_3 = 2U,
    SHADER_UNIFORM_TYPE_FLOAT32_4 = 3U,
    SHADER_UNIFORM_TYPE_INT8 = 4U,
    SHADER_UNIFORM_TYPE_UINT8 = 5U,
    SHADER_UNIFORM_TYPE_INT16 = 6U,
    SHADER_UNIFORM_TYPE_UINT16 = 7U,
    SHADER_UNIFORM_TYPE_INT32 = 8U,
    SHADER_UNIFORM_TYPE_UINT32 = 9U,
    SHADER_UNIFORM_TYPE_MATRIX_4 = 10U,
    SHADER_UNIFORM_TYPE_SAMPLER = 11U,
    SHADER_UNIFORM_TYPE_CUSTOM = 255U
} E_SHADER_UNIFORM_TYPE;

/**
 * @brief Defines shader scope, which indicates how
 * often it gets updated.
 */
typedef enum SHADER_SCOPE {
    /** @brief Global shader scope, generally updated once per frame. */
    SHADER_SCOPE_GLOBAL = 0,
    /** @brief Instance shader scope, generally updated "per-instance" of the shader. */
    SHADER_SCOPE_INSTANCE = 1,
    /** @brief Local shader scope, generally updated per-object */
    SHADER_SCOPE_LOCAL = 2
} SHADER_SCOPE;

/** @brief Configuration for an attribute. */
typedef struct SHADER_ATTRIBUTE_CONFIG {
    /** @brief The length of the name. */
    u8 name_length;
    /** @brief The name of the attribute. */
    char* name;
    /** @brief The size of the attribute. */
    u8 size;
    /** @brief The type of the attribute. */
    E_SHADER_ATTRIBUTE_TYPE type;
} SHADER_ATTRIBUTE_CONFIG;

/** @brief Configuration for a uniform. */
typedef struct SHADER_UNIFORM_CONFIG {
    /** @brief The length of the name. */
    u8 name_length;
    /** @brief The name of the uniform. */
    char* name;
    /** @brief The size of the uniform. */
    u8 size;
    /** @brief The location of the uniform. */
    u32 location;
    /** @brief The type of the uniform. */
    E_SHADER_UNIFORM_TYPE type;
    /** @brief The scope of the uniform. */
    SHADER_SCOPE scope;
} SHADER_UNIFORM_CONFIG;

/**
 * @brief Configuration for a shader. Typically created and
 * destroyed by the shader resource loader, and set to the
 * properties found in a .shadercfg resource file.
 */
typedef struct SHADER_CONFIG {
    /** @brief The name of the shader to be created. */
    char* name;

    /** @brief Indicates if the shader uses instance-level uniforms. */
    b8 use_instances;
    /** @brief Indicates if the shader uses local-level uniforms. */
    b8 use_local;

    /** @brief The count of attributes. */
    u8 attribute_count;
    /** @brief The collection of attributes. Darray. */
    SHADER_ATTRIBUTE_CONFIG* attributes;

    /** @brief The count of uniforms. */
    u8 uniform_count;
    /** @brief The collection of uniforms. Darray. */
    SHADER_UNIFORM_CONFIG* uniforms;

    /** @brief The name of the renderpass used by this shader. */
    char* renderpass_name;

    /** @brief The number of stages present in the shader. */
    u8 stage_count;
    /** @brief The collection of stages. Darray. */
    E_SHADER_STAGE* stages;
    /** @brief The collection of stage names. Must align with stages array. Darray. */
    char** stage_names;
    /** @brief The collection of stage file names to be loaded (one per stage). Must align with stages array. Darray. */
    char** vulkan_stage_filenames;
    char** webgpu_stage_filenames;
} SHADER_CONFIG;
