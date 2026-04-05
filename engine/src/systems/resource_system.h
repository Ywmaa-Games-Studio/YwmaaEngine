#pragma once

#include "resources/resource_types.h"

typedef struct RESOURCE_SYSTEM_CONFIG {
    u32 max_loader_count;
    // The relative base path for assets.
    char* asset_base_path;
} RESOURCE_SYSTEM_CONFIG;

typedef struct RESOURCE_LOADER {
    u32 id;
    E_RESOURCE_TYPE type;
    const char* custom_type;
    const char* type_path;
    b8 (*load)(struct RESOURCE_LOADER* self, const char* name, void* params, RESOURCE* out_resource);
    void (*unload)(struct RESOURCE_LOADER* self, RESOURCE* resource);
} RESOURCE_LOADER;

b8 resource_system_init(u64* memory_requirement, void* state, void* config);
void resource_system_shutdown(void* state);

YAPI b8 resource_system_loader_register(RESOURCE_LOADER loader);

YAPI b8 resource_system_load(const char* name, E_RESOURCE_TYPE type, void* params, RESOURCE* out_resource);
YAPI b8 resource_system_load_custom(const char* name, const char* custom_type, void* params, RESOURCE* out_resource);

YAPI void resource_system_unload(RESOURCE* resource);

YAPI const char* resource_system_base_path(void);
