#pragma once

#include "math/math_types.h"

// Pre-defined resource types.
typedef enum E_RESOURCE_TYPE {
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_MESH,
    RESOURCE_TYPE_BITMAP_FONT,
    RESOURCE_TYPE_SYSTEM_FONT,
    RESOURCE_TYPE_CUSTOM
} E_RESOURCE_TYPE;

/** @brief A magic number indicating the file as a ywmaa engine binary file. */
#define RESOURCE_MAGIC 0xcafef00d

/**
 * @brief The header data for binary resource types.
 */
typedef struct RESOURCE_HEADER {
    /** @brief A magic number indicating the file as a ywmaa engine binary file. */
    u32 magic_number;
    /** @brief The resource type. Maps to the enum resource_type. */
    u8 resource_type;
    /** @brief The format version this resource uses. */
    u8 version;
    /** @brief Reserved for future header data.. */
    u16 reserved;
} RESOURCE_HEADER;

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

/** @brief Parameters used when loading an image. */
typedef struct IMAGE_RESOURCE_PARAMS {
    /** @brief Indicates if the image should be flipped on the y-axis when loaded. */
    b8 flip_y;
} IMAGE_RESOURCE_PARAMS;

/** @brief Determines face culling mode during rendering. */
typedef enum E_FACE_CULL_MODE {
    /** @brief No faces are culled. */
    FACE_CULL_MODE_NONE = 0x0,
    /** @brief Only front faces are culled. */
    FACE_CULL_MODE_FRONT = 0x1,
    /** @brief Only back faces are culled. */
    FACE_CULL_MODE_BACK = 0x2,
    /** @brief Both front and back faces are culled. */
    FACE_CULL_MODE_FRONT_AND_BACK = 0x3
} E_FACE_CULL_MODE;

#define TEXTURE_NAME_MAX_LENGTH 512

typedef enum E_TEXTURE_FLAG {
    /** @brief Indicates if the texture has transparency. */
    TEXTURE_FLAG_HAS_TRANSPARENCY = 0x1,
    /** @brief Indicates if the texture can be written (rendered) to. */
    TEXTURE_FLAG_IS_WRITEABLE = 0x2,
    /** @brief Indicates if the texture was created via wrapping vs traditional creation. */
    TEXTURE_FLAG_IS_WRAPPED = 0x4,
    /** @brief Indicates the texture is a depth texture. */
    TEXTURE_FLAG_DEPTH = 0x8
} E_TEXTURE_FLAG;

/** @brief Holds bit flags for textures.. */
typedef u8 texture_flag_bits;

/**
 * @brief Represents various types of textures.
 */
typedef enum E_TEXTURE_TYPE {
    /** @brief A standard two-dimensional texture. */
    TEXTURE_TYPE_2D,
    /** @brief A cube texture, used for cubemaps. */
    TEXTURE_TYPE_CUBE
} E_TEXTURE_TYPE;

typedef struct TEXTURE {
    u32 id;
    E_TEXTURE_TYPE type;
    u32 width;
    u32 height;
    u8 channel_count;
    /** @brief Holds various flags for this texture. */
    texture_flag_bits flags;
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* internal_data;
} TEXTURE;

typedef enum E_TEXTURE_USE {
    TEXTURE_USE_UNKNOWN = 0x00,
    TEXTURE_USE_MAP_DIFFUSE = 0x01,
    TEXTURE_USE_MAP_SPECULAR = 0x02,
    TEXTURE_USE_MAP_NORMAL = 0x03,
    TEXTURE_USE_MAP_CUBEMAP = 0x04,
} E_TEXTURE_USE;

/** @brief Represents supported texture filtering modes. */
typedef enum E_TEXTURE_FILTER {
    /** @brief Nearest-neighbor filtering. */
    TEXTURE_FILTER_MODE_NEAREST = 0x0,
    /** @brief Linear (i.e. bilinear) filtering.*/
    TEXTURE_FILTER_MODE_LINEAR = 0x1
} E_TEXTURE_FILTER;

typedef enum E_TEXTURE_REPEAT {
    TEXTURE_REPEAT_REPEAT = 0x1,
    TEXTURE_REPEAT_MIRRORED_REPEAT = 0x2,
    TEXTURE_REPEAT_CLAMP_TO_EDGE = 0x3,
    TEXTURE_REPEAT_CLAMP_TO_BORDER = 0x4
} E_TEXTURE_REPEAT;

typedef struct TEXTURE_MAP {
    TEXTURE* texture;
    E_TEXTURE_USE use;
    /** @brief Texture filtering mode for minification. */
    E_TEXTURE_FILTER filter_minify;
    /** @brief Texture filtering mode for magnification. */
    E_TEXTURE_FILTER filter_magnify;
    /** @brief The repeat mode on the U axis (or X, or S) */
    E_TEXTURE_REPEAT repeat_u;
    /** @brief The repeat mode on the V axis (or Y, or T) */
    E_TEXTURE_REPEAT repeat_v;
    /** @brief The repeat mode on the W axis (or Z, or U) */
    E_TEXTURE_REPEAT repeat_w;
    /** @brief A pointer to internal, render API-specific data. Typically the internal sampler. */
    void* internal_data;
} TEXTURE_MAP;

typedef struct FONT_GLYPH {
    i32 codepoint;
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    i16 x_offset;
    i16 y_offset;
    i16 x_advance;
    u8 page_id;
} FONT_GLYPH;

typedef struct FONT_KERNING {
    i32 codepoint_0;
    i32 codepoint_1;
    i16 amount;
} FONT_KERNING;

typedef enum E_FONT_TYPE {
    FONT_TYPE_BITMAP,
    FONT_TYPE_SYSTEM
} E_FONT_TYPE;

typedef struct FONT_DATA {
    E_FONT_TYPE type;
    char face[256];
    u32 size;
    i32 line_height;
    i32 baseline;
    i32 atlas_size_x;
    i32 atlas_size_y;
    TEXTURE_MAP atlas;
    u32 glyph_count;
    FONT_GLYPH* glyphs;
    u32 kerning_count;
    FONT_KERNING* kernings;
    f32 tab_x_advance;
    u32 internal_data_size;
    void* internal_data;
} FONT_DATA;

typedef struct BITMAP_FONT_PAGE {
    i8 id;
    char file[256];
} BITMAP_FONT_PAGE;

typedef struct BITMAP_FONT_RESOURCE_DATA {
    FONT_DATA data;
    u32 page_count;
    BITMAP_FONT_PAGE* pages;
} BITMAP_FONT_RESOURCE_DATA;

typedef struct SYSTEM_FONT_FACE {
    char name[256];
} SYSTEM_FONT_FACE;

typedef struct SYSTEM_FONT_RESOURCE_DATA {
    // darray
    SYSTEM_FONT_FACE* fonts;
    u64 binary_size;
    void* font_binary;
} SYSTEM_FONT_RESOURCE_DATA;

#define MATERIAL_NAME_MAX_LENGTH 256
typedef struct MATERIAL_CONFIG {
    char name[MATERIAL_NAME_MAX_LENGTH];
    char* shader_name;
    b8 auto_release;
    Vector4 diffuse_color;
    /** @brief The shiness of the material. */
    f32 shiness;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
    char specular_map_name[TEXTURE_NAME_MAX_LENGTH];
    char normal_map_name[TEXTURE_NAME_MAX_LENGTH];
} MATERIAL_CONFIG;

typedef struct MATERIAL {
    u32 id;
    u32 generation;
    u32 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vector4 diffuse_color;
    TEXTURE_MAP diffuse_map;
    TEXTURE_MAP specular_map;
    TEXTURE_MAP normal_map;
    /** @brief The material shiness, determines how concentrated the specular lighting is. */
    f32 shiness;

    u32 shader_id;

    /** @brief Synced to the renderer's current frame number when the material has been applied that frame. */
    u32 render_frame_number;
} MATERIAL;


#define GEOMETRY_NAME_MAX_LENGTH 256

/**
 * @brief Represents actual geometry in the world.
 * Typically (but not always, depending on use) paired with a material.
 */
typedef struct GEOMETRY {
    u32 id;
    u32 internal_id;
    u16 generation;
    /** @brief The center of the geometry in local coordinates. */
    Vector3 center;
    /** @brief The extents of the geometry in local coordinates. */
    Extents3D extents;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    MATERIAL* material;
} GEOMETRY;

typedef struct Mesh {
    u32 unique_id;
    u8 generation;
    u16 geometry_count;
    GEOMETRY** geometries;
    Transform transform;
} Mesh;

typedef struct Skybox {
    TEXTURE_MAP cubemap;
    GEOMETRY* g;
    u32 instance_id;
    /** @brief Synced to the renderer's current frame number when the material has been applied that frame. */
    u64 render_frame_number;
} Skybox;

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
    SHADER_UNIFORM_TYPE_CUBE_SAMPLER = 12U,
    SHADER_UNIFORM_TYPE_CUSTOM = 255U
} E_SHADER_UNIFORM_TYPE;

/**
 * @brief Defines shader scope, which indicates how
 * often it gets updated.
 */
typedef enum E_SHADER_SCOPE {
    /** @brief Global shader scope, generally updated once per frame. */
    SHADER_SCOPE_GLOBAL = 0,
    /** @brief Instance shader scope, generally updated "per-instance" of the shader. */
    SHADER_SCOPE_INSTANCE = 1,
    /** @brief Local shader scope, generally updated per-object */
    SHADER_SCOPE_LOCAL = 2
} E_SHADER_SCOPE;

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
    E_SHADER_SCOPE scope;
} SHADER_UNIFORM_CONFIG;

/**
 * @brief Configuration for a shader. Typically created and
 * destroyed by the shader resource loader, and set to the
 * properties found in a .shadercfg resource file.
 */
typedef struct SHADER_CONFIG {
    /** @brief The name of the shader to be created. */
    char* name;

    /** @brief The face cull mode to be used. Default is BACK if not supplied. */
    E_FACE_CULL_MODE cull_mode;

    /** @brief The count of attributes. */
    u8 attribute_count;
    /** @brief The collection of attributes. Darray. */
    SHADER_ATTRIBUTE_CONFIG* attributes;

    /** @brief The count of uniforms. */
    u8 uniform_count;
    /** @brief The collection of uniforms. Darray. */
    SHADER_UNIFORM_CONFIG* uniforms;

    /** @brief The number of stages present in the shader. */
    u8 stage_count;
    /** @brief The collection of stages. Darray. */
    E_SHADER_STAGE* stages;
    /** @brief The collection of stage names. Must align with stages array. Darray. */
    char** stage_names;
    /** @brief The collection of stage file names to be loaded (one per stage). Must align with stages array. Darray. */
    char** vulkan_stage_filenames;
    char** webgpu_stage_filenames;

    // TODO: Convert these bools to flags.
    /** @brief Indicates if depth testing should be done. */
    b8 depth_test;
    /**
     * @brief Indicates if the results of depth testing should be written to the depth buffer.
     * NOTE: This is ignored if depth_test is false.
     */
    b8 depth_write;
} SHADER_CONFIG;
