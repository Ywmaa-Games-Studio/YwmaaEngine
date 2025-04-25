#include "dynamic_allocator.h"

#include "core/ymemory.h"
#include "core/logger.h"
#include "memory/freelist.h"
#include "core/asserts.h"
#include "stdalign.h" // For alignof
typedef struct dynamic_allocator_state {
    u64 total_size;
    FREELIST list;
    void* freelist_block;
    void* memory_block;
} dynamic_allocator_state;

typedef struct alloc_header {
    u32 size;
    u16 alignment;
    void* start;
} alloc_header;

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
    if (!allocator) {
        PRINT_WARNING("dynamic_allocator_destroy requires a pointer to an allocator. Destroy failed.");
        return false;
    }

    dynamic_allocator_state* state = allocator->memory;
    freelist_destroy(&state->list);
    yzero_memory(state->memory_block, state->total_size);
    state->total_size = 0;
    allocator->memory = 0;
    return true;
}

void* dynamic_allocator_allocate(DYNAMIC_ALLOCATOR* allocator, u64 size) {
    return dynamic_allocator_allocate_aligned(allocator, size, 1);
}

// the actual alignment is actually at minimum alignof(alloc_header)
void* dynamic_allocator_allocate_aligned(DYNAMIC_ALLOCATOR* allocator, u64 size, u16 alignment) {
    if (!allocator || !size || !alignment) {
        PRINT_ERROR("dynamic_allocator_allocate_aligned requires a valid allocator, size and alignment.");
        return 0;
    }

    // Ensure alignment is power of 2
    if ((alignment & (alignment - 1)) != 0) {
        PRINT_ERROR("Alignment must be power of 2");
    }

    dynamic_allocator_state* state = allocator->memory;

    
    // The size required is based on the requested size, plus the alignment, header
    u64 header_size = sizeof(alloc_header);
    u64 header_alignment = alignof(alloc_header);
    // Calculate total required space
    u64 required_alignment = alignment > header_alignment ? alignment : header_alignment;
    const u64 required_size = 
        required_alignment +    // alignment
        header_size +           // Header
        size;                   // User data
    // NOTE: This cast will really only be an issue on allocations over ~4GiB, so... don't do that.
    YASSERT_MSG(required_size < 4294967295U, "dynamic_allocator_allocate_aligned called with required size > 4 GiB. Don't do that.");

    u64 base_offset = 0;
    if (!freelist_allocate_block(&state->list, required_size, &base_offset)) {
        PRINT_ERROR("dynamic_allocator_allocate_aligned no blocks of memory large enough to allocate from.");
        u64 available = freelist_free_space(&state->list);
        PRINT_ERROR("Requested size: %llu, total space available: %llu", size, available);
        // TODO: Report fragmentation?
        return 0;
    }
    /*
    Memory layout:
    x bytes/void padding
    alloc_header
    x bytes/void user memory block

    */
    // Get the base pointer, or the unaligned memory block.
    void* ptr = (void*)((u64)state->memory_block + base_offset);
    // Align the header first
    u64 header_position = get_aligned((u64)ptr, required_alignment);
    alloc_header* header = (alloc_header*)header_position;
    header->start = ptr;
    YASSERT_MSG(header->start, "dynamic_allocator_allocate_aligned got a null pointer (0x0). Memory corruption likely as this should always be nonzero.");
    header->alignment = required_alignment;
    YASSERT_MSG(header->alignment, "dynamic_allocator_allocate_aligned got an alignment of 0. Memory corruption likely as this should always be nonzero.");
    header->size = (u32)size;
    YASSERT_MSG(header->size, "dynamic_allocator_allocate_aligned got a size of 0. Memory corruption likely as this should always be nonzero.");
    
    // Align the user block after the header.
    u64 aligned_block_offset = header_position + sizeof(alloc_header);
    //PRINT_DEBUG("Allocated block at %u, size: %u, alignment: %u, header at %u", aligned_block_offset, size, required_alignment, header_position);
    return (void*)aligned_block_offset;
}

b8 dynamic_allocator_free(DYNAMIC_ALLOCATOR* allocator, void* block, u64 size) {
    return dynamic_allocator_free_aligned(allocator, block);
}

b8 dynamic_allocator_free_aligned(DYNAMIC_ALLOCATOR* allocator, void* block) {
    if (!allocator || !block) {
        PRINT_ERROR("dynamic_allocator_free_aligned requires both a valid allocator (0x%p) and a block (0x%p) to be freed.", allocator, block);
        return false;
    }

    dynamic_allocator_state* state = allocator->memory;
    if (block < state->memory_block || block > state->memory_block + state->total_size) {
        void* end_of_block = (void*)(state->memory_block + state->total_size);
        PRINT_WARNING("dynamic_allocator_free_aligned trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p)", block, state->memory_block, end_of_block);
        return false;
    }

    u64 header_address = ((u64)block - sizeof(alloc_header));
    alloc_header* header = (alloc_header*)header_address;
    u64 required_size = header->alignment + header->size + sizeof(alloc_header);
    u64 offset = (u64)header->start - (u64)state->memory_block;

    //PRINT_DEBUG("Freeing block at 0x%p, stored size: %u, stored alignment: %u", block, *block_size, header->alignment);
    if (!freelist_free_block(&state->list, required_size, offset)) {
        PRINT_ERROR("dynamic_allocator_free_aligned failed.");
        return false;
    }

    return true;
}

b8 dynamic_allocator_get_size_alignment(DYNAMIC_ALLOCATOR* allocator, void* block, u64* out_size, u16* out_alignment) {
    if (!allocator || !block || !out_size || !out_alignment) {
        PRINT_ERROR("dynamic_allocator_get_size_alignment requires non-null inputs.");
        return false;
    }

    dynamic_allocator_state* state = allocator->memory;
    if (block < state->memory_block || block >= ((void*)((u8*)state->memory_block) + state->total_size)) {
        // Not owned by this block.
        return false;
    }

    // Get the header.
    u64 header_position = ((u64)block - sizeof(alloc_header));
    alloc_header* header = (alloc_header*)header_position;
    u64 header_alignment = alignof(alloc_header);
    *out_size = header->size;
    *out_alignment = header->alignment;
    YASSERT_MSG(*out_size, "dynamic_allocator_get_size_alignment found an out_size of 0. Memory corruption likely.");
    YASSERT_MSG(*out_alignment, "dynamic_allocator_get_size_alignment found a header->alignment of 0. Memory corruption likely as this should always be at least 1.");
    return true;
}

u64 dynamic_allocator_free_space(DYNAMIC_ALLOCATOR* allocator) {
    dynamic_allocator_state* state = allocator->memory;
    return freelist_free_space(&state->list);
}

u64 dynamic_allocator_total_space(DYNAMIC_ALLOCATOR* allocator) {
    dynamic_allocator_state* state = allocator->memory;
    return state->total_size;
}
