
#pragma once

#include "defines.h"

typedef struct DYNAMIC_ALLOCATOR {
    void* memory;
} DYNAMIC_ALLOCATOR;

YAPI b8 dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory, DYNAMIC_ALLOCATOR* out_allocator);

YAPI b8 dynamic_allocator_destroy(DYNAMIC_ALLOCATOR* allocator);

YAPI void* dynamic_allocator_allocate(DYNAMIC_ALLOCATOR* allocator, u64 size);

YAPI void* dynamic_allocator_allocate_aligned(DYNAMIC_ALLOCATOR* allocator, u64 size, u16 alignment);

YAPI b8 dynamic_allocator_free(DYNAMIC_ALLOCATOR* allocator, void* block, u64 size);

YAPI b8 dynamic_allocator_free_aligned(DYNAMIC_ALLOCATOR* allocator, void* block);

YAPI b8 dynamic_allocator_get_size_alignment(DYNAMIC_ALLOCATOR* allocator, void* block, u64* out_size, u16* out_alignment);

YAPI u64 dynamic_allocator_free_space(DYNAMIC_ALLOCATOR* allocator);

YAPI u64 dynamic_allocator_total_space(DYNAMIC_ALLOCATOR* allocator);