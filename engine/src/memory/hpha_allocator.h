/* HPHA ALLOCATOR.h
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.23 -04:59
 * Last edited:
 *   2025.04.24 -23:42
 * Auto updated?
 *   Yes
 *
 * Description:
 *   HPHA Allocator
**/

#pragma once
#include "dynamic_allocator.h"
#include "defines.h"

// this is a basic implementation of the HPHA allocator(High-Performance Heap Allocator) inspired by O3DE
// currently it just creates different pools using the dynamic allocator, each uses a freelist.
// but in the future I might need to tweak the pool system perhaps if the freelists aren't enough.
// and currently the HPHA allocator is not thread safe.
// also adding a true object pool system would be a good idea.
// and the fallback should probably be a custom allocator instead of the platform one.
// perhaphs we allocate double the size of the current total size each time we run out of memory.
// or allocate small chunks of memory which size is set by the engine user in the config.
// this is a work in progress and should be treated as such.

// Size class thresholds (adjust as needed)
#define HPHA_SMALL_MAX (KIBIBYTES(1))    // 1KB
#define HPHA_MEDIUM_MAX (MEBIBYTES(1)) // 1MB

// Default allocation percentages
#define HPHA_DEFAULT_SMALL_PCT  30  // 20% for small allocations (<=1KB)
#define HPHA_DEFAULT_MEDIUM_PCT 50  // 50% for medium allocations (4KB-1MB)
#define HPHA_DEFAULT_LARGE_PCT  20  // 30% for large allocations (>1MB)

typedef struct HPHA_CONFIG {
    u8 small_percentage;   // Percentage for small allocations
    u8 medium_percentage;  // Percentage for medium allocations
    u8 large_percentage;   // Percentage for large allocations
} HPHA_CONFIG;

typedef struct HPHA_ALLOCATOR {
    // Memory tracking
    u64 total_size;
    u64 small_size;
    u64 medium_size;
    u64 large_size;
    DYNAMIC_ALLOCATOR small_allocator;
    DYNAMIC_ALLOCATOR medium_allocator;
    DYNAMIC_ALLOCATOR large_allocator;
    void* memory_block;
} HPHA_ALLOCATOR;

YAPI b8 hpha_allocator_create(u64 total_size, HPHA_CONFIG config, u64* memory_requirement, void* memory, HPHA_ALLOCATOR* out_allocator);
YAPI void hpha_allocator_destroy(HPHA_ALLOCATOR* allocator);
YAPI void* hpha_allocate(HPHA_ALLOCATOR* allocator, u64 size, u16 alignment);
YAPI b8 hpha_free(HPHA_ALLOCATOR* allocator, void* block);

YAPI void hpha_allocator_free_space(HPHA_ALLOCATOR* allocator, u64* out_small_free, u64* out_medium_free, u64* out_large_free);

YAPI void hpha_allocator_total_space(HPHA_ALLOCATOR* allocator, u64* out_small_total, u64* out_medium_total, u64* out_large_total);

YAPI b8 hpha_get_size_alignment(HPHA_ALLOCATOR* allocator, void* block, u64* out_size, u16* out_alignment);