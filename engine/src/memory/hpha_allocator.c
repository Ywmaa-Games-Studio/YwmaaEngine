/* HPHA ALLOCATOR.c
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.23 -04:59
 * Last edited:
 *   <l1741PMle>
 * Auto updated?
 *   Yes
 *
 * Description:
 *   HPHA Allocator
**/

#include "hpha_allocator.h"
#include "core/logger.h"
#include "platform/platform.h"

b8 hpha_allocator_create(u64 total_size, HPHA_CONFIG config, u64* memory_requirement, void* memory, HPHA_ALLOCATOR* out_allocator) {
    // Validate percentages
    if (config.small_percentage + config.medium_percentage + config.large_percentage != 100) {
        PRINT_ERROR("HPHA allocation percentages must sum to 100");
        return false;
    }

    // Calculate pool sizes
    u64 small_pool = (total_size * config.small_percentage) / 100;
    small_pool = ((small_pool/64)+1)*64;
    u64 medium_pool = (total_size * config.medium_percentage) / 100;
    medium_pool = ((medium_pool/64)+1)*64;
    u64 large_pool = total_size - small_pool - medium_pool; // Ensure no rounding errors

    // Calculate requirements for each allocator
    u64 small_req = 0, medium_req = 0, large_req = 0;
    dynamic_allocator_create(small_pool, &small_req, 0, 0);
    dynamic_allocator_create(medium_pool, &medium_req, 0, 0);
    dynamic_allocator_create(large_pool, &large_req, 0, 0);

    // Total requirement = state + all allocator blocks
    *memory_requirement = sizeof(HPHA_ALLOCATOR) + small_req + medium_req + large_req;

    if (!memory) return true; // Just querying requirements

    // Initialize
    HPHA_ALLOCATOR* allocator = (HPHA_ALLOCATOR*)memory;
    allocator->total_size = total_size;
    allocator->small_size = small_pool;
    allocator->medium_size = medium_pool;
    allocator->large_size = large_pool;

    // Memory layout: state followed by allocator blocks
    u8* mem = (u8*)memory + sizeof(HPHA_ALLOCATOR);
    void* small_block = mem;
    void* medium_block = mem + small_req;
    void* large_block = mem + small_req + medium_req;

    if (!dynamic_allocator_create(small_pool, &small_req, small_block, &allocator->small_allocator) ||
        !dynamic_allocator_create(medium_pool, &medium_req, medium_block, &allocator->medium_allocator) ||
        !dynamic_allocator_create(large_pool, &large_req, large_block, &allocator->large_allocator)) {
        PRINT_ERROR("Failed to initialize HPHA sub-allocators");
        return false;
    }

    allocator->memory_block = memory;
    if (out_allocator) *out_allocator = *allocator;
    return true;
}

void hpha_allocator_destroy(HPHA_ALLOCATOR* allocator) {
    if (!allocator) return;

    // Destroy allocators (no need to free blocks - they're part of the main memory block)
    dynamic_allocator_destroy(&allocator->small_allocator);
    dynamic_allocator_destroy(&allocator->medium_allocator);
    dynamic_allocator_destroy(&allocator->large_allocator);
}

void* hpha_allocate(HPHA_ALLOCATOR* allocator, u64 size, u16 alignment, u64* allocated_size) {
    if (!allocator) return 0;

    DYNAMIC_ALLOCATOR* target = 0;
    if (size <= HPHA_SMALL_MAX) {
        target = &allocator->small_allocator;
    } else if (size <= HPHA_MEDIUM_MAX) {
        target = &allocator->medium_allocator;
    } else {
        target = &allocator->large_allocator;
    }

    void* block = dynamic_allocator_allocate_aligned(target, size, alignment);
    if (!block) {
        // Fallback to platform allocator
        PRINT_WARNING("HPHA allocation failed for size %llu. Falling back to platform allocator.", size);
        block = platform_allocate(size, alignment);
    }
    dynamic_allocator_get_size(target, block, allocated_size);
    return block;
}

b8 hpha_free(HPHA_ALLOCATOR* allocator, void* block, u64* freed_size) {
    if (!allocator || !block) return false;
    
    // Try each allocator to find the owner
    u64 size;
    if (dynamic_allocator_get_size(&allocator->small_allocator, block, &size)) {
        *freed_size = size;
        return dynamic_allocator_free(&allocator->small_allocator, block);
    }
    if (dynamic_allocator_get_size(&allocator->medium_allocator, block, &size)) {
        *freed_size = size;
        return dynamic_allocator_free(&allocator->medium_allocator, block);
    }
    if (dynamic_allocator_get_size(&allocator->large_allocator, block, &size)) {
        *freed_size = size;
        return dynamic_allocator_free(&allocator->large_allocator, block);
    }
    freed_size = 0; //unknown

    // Not owned by any allocator - must be a platform fallback
    platform_free(block, false);
    return true;
}

void hpha_allocator_free_space(HPHA_ALLOCATOR* allocator, u64* out_small_free, u64* out_medium_free, u64* out_large_free) {
    if (out_small_free) *out_small_free = dynamic_allocator_free_space(&allocator->small_allocator);
    if (out_medium_free) *out_medium_free = dynamic_allocator_free_space(&allocator->medium_allocator);
    if (out_large_free) *out_large_free = dynamic_allocator_free_space(&allocator->large_allocator);
}

void hpha_allocator_total_space(HPHA_ALLOCATOR* allocator, u64* out_small_total, u64* out_medium_total, u64* out_large_total) {
    if (out_small_total) *out_small_total = dynamic_allocator_total_space(&allocator->small_allocator);
    if (out_medium_total) *out_medium_total = dynamic_allocator_total_space(&allocator->medium_allocator);
    if (out_large_total) *out_large_total = dynamic_allocator_total_space(&allocator->large_allocator);
}

b8 hpha_get_size(HPHA_ALLOCATOR* allocator, void* block, u64* out_size) {
    if (!allocator || !block) {
        PRINT_ERROR("hpha_get_size requires valid allocator and block");
        return false;
    }

    // Check if block belongs to any sub-allocator
    if (dynamic_allocator_get_size(&allocator->small_allocator, block, out_size)) {
        return true;
    }
    if (dynamic_allocator_get_size(&allocator->medium_allocator, block, out_size)) {
        return true;
    }
    if (dynamic_allocator_get_size(&allocator->large_allocator, block, out_size)) {
        return true;
    }

    // Block not owned by HPHA (could be a platform fallback allocation)
    return false;
}

