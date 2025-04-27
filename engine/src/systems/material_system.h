#pragma once

#include "defines.h"

#include "resources/resource_types.h"

#define DEFAULT_MATERIAL_NAME "default"

typedef struct MATERIAL_SYSTEM_CONFIG {
    u32 max_material_count;
} MATERIAL_SYSTEM_CONFIG;

typedef struct MATERIAL_CONFIG {
    char name[MATERIAL_NAME_MAX_LENGTH];
    b8 auto_release;
    Vector4 diffuse_colour;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} MATERIAL_CONFIG;

b8 material_system_init(u64* memory_requirement, void* state, MATERIAL_SYSTEM_CONFIG config);
void material_system_shutdown(void* state);

MATERIAL* material_system_acquire(const char* name);
MATERIAL* material_system_acquire_from_config(MATERIAL_CONFIG config);
void material_system_release(const char* name);

MATERIAL* material_system_get_default(void);
