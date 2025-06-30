#include "linear_allocator.h"

#include "core/ymemory.h"
#include "core/logger.h"

void linear_allocator_create(u64 total_size, void* memory, LINEAR_ALLOCATOR* out_allocator) {
    if (out_allocator) {
        out_allocator->total_size = total_size;
        out_allocator->allocated = 0;
        out_allocator->owns_memory = memory == 0;
        if (memory) {
            out_allocator->memory = memory;
        } else {
            out_allocator->memory = yallocate(total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
        }
    }
}
void linear_allocator_destroy(LINEAR_ALLOCATOR* allocator) {
    if (allocator) {
        allocator->allocated = 0;
        if (allocator->owns_memory && allocator->memory) {
            yfree(allocator->memory);
        } 
        allocator->memory = 0;
        allocator->total_size = 0;
        allocator->owns_memory = false;
    }
}

void* linear_allocator_allocate(LINEAR_ALLOCATOR* allocator, u64 size) {
    if (allocator && allocator->memory) {
        if (allocator->allocated + size > allocator->total_size) {
            u64 remaining = allocator->total_size - allocator->allocated;
            PRINT_ERROR("linear_allocator_allocate - Tried to allocate %lluB, only %lluB remaining.", size, remaining);
            return 0;
        }

        void* block = ((u8*)allocator->memory) + allocator->allocated;
        allocator->allocated += size;
        return block;
    }

    PRINT_ERROR("linear_allocator_allocate - provided allocator not initialized.");
    return 0;
}

void linear_allocator_free_all(LINEAR_ALLOCATOR* allocator) {
    if (allocator && allocator->memory) {
        allocator->allocated = 0;
        yzero_memory(allocator->memory, allocator->total_size);
    }
}
