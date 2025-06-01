#pragma once

#include "renderer/renderer_types.inl"

typedef struct TEXTURE_SYSTEM_CONFIG {
    u32 max_texture_count;
} TEXTURE_SYSTEM_CONFIG;

#define DEFAULT_TEXTURE_NAME "default"
/** @brief The default specular texture name. */
#define DEFAULT_SPECULAR_TEXTURE_NAME "default_SPEC"
/** @brief The default normal texture name. */
#define DEFAULT_NORMAL_TEXTURE_NAME "default_NORM"

b8 texture_system_init(u64* memory_requirement, void* state, TEXTURE_SYSTEM_CONFIG config);
void texture_system_shutdown(void* state);

TEXTURE* texture_system_acquire(const char* name, b8 auto_release);
void texture_system_release(const char* name);

TEXTURE* texture_system_get_default_texture(void);
TEXTURE* texture_system_get_default_specular_texture(void);
TEXTURE* texture_system_get_default_normal_texture(void);
