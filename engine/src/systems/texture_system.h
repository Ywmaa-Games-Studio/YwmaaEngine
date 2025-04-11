#pragma once

#include "renderer/renderer_types.inl"

typedef struct TEXTURE_SYSTEM_CONFIG {
    u32 max_texture_count;
} TEXTURE_SYSTEM_CONFIG;

#define DEFAULT_TEXTURE_NAME "default"

b8 texture_system_init(u64* memory_requirement, void* state, TEXTURE_SYSTEM_CONFIG config);
void texture_system_shutdown(void* state);

TEXTURE* texture_system_acquire(const char* name, b8 auto_release);
void texture_system_release(const char* name);

TEXTURE* texture_system_get_default_texture();