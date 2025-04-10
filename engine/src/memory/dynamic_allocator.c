#include "dynamic_allocator.h"

#include "core/ymemory.h"
#include "core/logger.h"
#include "variants/freelist.h"

typedef struct dynamic_allocator_state {
    u64 total_size;
    FREELIST list;
    void* freelist_block;
    void* memory_block;
} dynamic_allocator_state;

b8 dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory, DYNAMIC_ALLOCATOR* out_allocator) {
    if (total_size < 1) {
        PRINT_ERROR("dynamic_allocator_create cannot have a total_size of 0. Create failed.");
        return false;
    }
    if (!memory_requirement) {
        PRINT_ERROR("dynamic_allocator_create requires memory_requirement to exist. Create failed.");
        return false;
    }
    u64 freelist_requirement = 0;
    // Grab the memory requirement for the free list first.
    freelist_create(total_size, &freelist_requirement, 0, 0);

    *memory_requirement = freelist_requirement + sizeof(dynamic_allocator_state) + total_size;

    // If only obtaining requirement, boot out.
    if (!memory) {
        return true;
    }

    // Memory layout:
    // state
    // FREELIST block
    // memory block
    out_allocator->memory = memory;
    dynamic_allocator_state* state = out_allocator->memory;
    state->total_size = total_size;
    state->freelist_block = (void*)(out_allocator->memory + sizeof(dynamic_allocator_state));
    state->memory_block = (void*)(state->freelist_block + freelist_requirement);

    // Actually create the FREELIST
    freelist_create(total_size, &freelist_requirement, state->freelist_block, &state->list);

    yzero_memory(state->memory_block, total_size);
    return true;
}

b8 dynamic_allocator_destroy(DYNAMIC_ALLOCATOR* allocator) {
    if (allocator) {
        dynamic_allocator_state* state = allocator->memory;
        freelist_destroy(&state->list);
        yzero_memory(state->memory_block, state->total_size);
        state->total_size = 0;
        return true;
    }

    PRINT_WARNING("dynamic_allocator_destroy requires a pointer to an allocator. Destroy failed.");
    return false;
}

void* dynamic_allocator_allocate(DYNAMIC_ALLOCATOR* allocator, u64 size) {
    if (allocator && size) {
        dynamic_allocator_state* state = allocator->memory;
        u64 offset = 0;
        // Attempt to allocate from the FREELIST.
        if (freelist_allocate_block(&state->list, size, &offset)) {
            // Use that offset against the base memory block to get the block.
            void* block = (void*)(state->memory_block + offset);
            return block;
        } else {
            PRINT_ERROR("dynamic_allocator_allocate no blocks of memory large enough to allocate from.");
            u64 available = freelist_free_space(&state->list);
            PRINT_ERROR("Requested size: %llu, total space available: %llu", size, available);
            // TODO: Report fragmentation?
            return 0;
        }
    }

    PRINT_ERROR("dynamic_allocator_allocate requires a valid allocator and size.");
    return 0;
}

b8 dynamic_allocator_free(DYNAMIC_ALLOCATOR* allocator, void* block, u64 size) {
    if (!allocator || !block || !size) {
        PRINT_ERROR("dynamic_allocator_free requires both a valid allocator (0x%p) and a block (0x%p) to be freed.", allocator, block);
        return false;
    }

    dynamic_allocator_state* state = allocator->memory;
    if (block < state->memory_block || block > state->memory_block + state->total_size) {
        void* end_of_block = (void*)(state->memory_block + state->total_size);
        PRINT_ERROR("dynamic_allocator_free trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p)", block, state->memory_block, end_of_block);
        return false;
    }
    u64 offset = (block - state->memory_block);
    if (!freelist_free_block(&state->list, size, offset)) {
        PRINT_ERROR("dynamic_allocator_free failed.");
        return false;
    }

    return true;
}

u64 dynamic_allocator_free_space(DYNAMIC_ALLOCATOR* allocator) {
    dynamic_allocator_state* state = allocator->memory;
    return freelist_free_space(&state->list);
}