#pragma once

#include "defines.h"

typedef struct LINEAR_ALLOCATOR {
    u64 total_size;
    u64 allocated;
    void* memory;
    b8 owns_memory;
} LINEAR_ALLOCATOR;

YAPI void linear_allocator_create(u64 total_size, void* memory, LINEAR_ALLOCATOR* out_allocator);
YAPI void linear_allocator_destroy(LINEAR_ALLOCATOR* allocator);

YAPI void* linear_allocator_allocate(LINEAR_ALLOCATOR* allocator, u64 size);
YAPI void linear_allocator_free_all(LINEAR_ALLOCATOR* allocator);