#include "ymemory.h"

#include "core/logger.h"
#include "core/ystring.h"
#include "core/asserts.h"
#include "platform/platform.h"
#include "memory/hpha_allocator.h"

// TODO: Custom string lib
#include <string.h>
#include <stdio.h>

#define Y_USE_CUSTOM_MEMORY_ALLOCATOR 1

#if !Y_USE_CUSTOM_MEMORY_ALLOCATOR
#    if _MSC_VER
#        include <malloc.h>
#        define yaligned_alloc _aligned_malloc
#        define yaligned_free _aligned_free
#    else
#        include <stdlib.h>
#        define yaligned_alloc(size, alignment) aligned_alloc(alignment, size)
#        define yaligned_free free
#    endif
#endif

struct MEMORY_STATS {
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
    u64 new_tagged_allocations[MEMORY_TAG_MAX_TAGS];
    u64 new_tagged_deallocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "LINEAR_ALLC",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "ROPE       ",
    "APPLICATION",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      ",
    "RESOURCE   "};

typedef struct MEMORY_SYSTEM_STATE {
    MEMORY_SYSTEM_CONFIG config;
    struct MEMORY_STATS stats;
    u64 alloc_count;
    u64 allocator_memory_requirement;
    HPHA_ALLOCATOR allocator;
    void* allocator_block;
} MEMORY_SYSTEM_STATE;

// Pointer to system state.
static MEMORY_SYSTEM_STATE* state_ptr;

b8 memory_system_init(MEMORY_SYSTEM_CONFIG config) {
#if Y_USE_CUSTOM_MEMORY_ALLOCATOR
    // The amount needed by the system state.
    u64 state_memory_requirement = sizeof(MEMORY_SYSTEM_STATE);

    // Configure HPHA with default percentages
    HPHA_CONFIG hpha_config = {
        .small_percentage = HPHA_DEFAULT_SMALL_PCT,
        .medium_percentage = HPHA_DEFAULT_MEDIUM_PCT,
        .large_percentage = HPHA_DEFAULT_LARGE_PCT
    };

    // Calculate HPHA requirements
    u64 alloc_requirement = 0;
    if (!hpha_allocator_create(config.total_alloc_size, hpha_config, &alloc_requirement, 0, 0)) {
        PRINT_ERROR("Failed to calculate HPHA requirements");
        return false;
    }

    // Call the platform allocator to get the memory for the whole system, including the state.
    void* block = platform_allocate(alloc_requirement, false);
    if (!block) {
        PRINT_ERROR("Memory system allocation failed and the system cannot continue.");
        return false;
    }

    // The state is in the first part of the massive block of memory.
    state_ptr = (MEMORY_SYSTEM_STATE*)block;
    state_ptr->config = config;
    state_ptr->alloc_count = 0;
    state_ptr->allocator_memory_requirement = alloc_requirement;
    platform_zero_memory(&state_ptr->stats, sizeof(state_ptr->stats));
    // The allocator block is in the same block of memory, but after the state.
    state_ptr->allocator_block = ((void*)block + state_memory_requirement);

    // Initialize HPHA
    if (!hpha_allocator_create(config.total_alloc_size, hpha_config, &alloc_requirement, state_ptr->allocator_block, &state_ptr->allocator)) {
        PRINT_ERROR("Failed to initialize HPHA");
        platform_free(state_ptr->allocator_block, false);
        return false;
    }
#else
    state_ptr = yaligned_alloc(sizeof(MEMORY_SYSTEM_STATE), 16);
    state_ptr->config = config;
    state_ptr->alloc_count = 0;
    state_ptr->allocator_memory_requirement = 0;
#endif

    PRINT_DEBUG("Memory system successfully allocated %llu bytes.", config.total_alloc_size);
    return true;
}

void memory_system_shutdown(void) {
    if (state_ptr) {
        hpha_allocator_destroy(&state_ptr->allocator);
        // Free the entire block.
        platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(MEMORY_SYSTEM_STATE));
    }
    state_ptr = 0;
}


void* yallocate(u64 size, E_MEMORY_TAG tag) {
    return yallocate_aligned(size, 1, tag);
}

void* yallocate_aligned(u64 size, u16 alignment, E_MEMORY_TAG tag) {
    YASSERT_MSG(size, "yallocate_aligned requires a nonzero size.");
    if (tag == MEMORY_TAG_UNKNOWN) {
        PRINT_WARNING("yallocate_aligned called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    if (!state_ptr) {
        // If the system is not up yet, warn about it but give memory for now.
        /* PRINT_TRACE("Warning: yallocate_aligned called before the memory system is initialized."); */
        // TODO: Memory alignment
        void* block = platform_allocate(size, false);
        return block;
    }

    // Track stats
    yallocate_report(size, tag);
    
#if Y_USE_CUSTOM_MEMORY_ALLOCATOR
    void* block = hpha_allocate(&state_ptr->allocator, size, alignment);
#else
    void* block = yaligned_alloc(size, alignment);
#endif
    // Use HPHA for allocation
    if (!block) {
        PRINT_ERROR("Allocation failed for size %llu", size);
        return 0;
    }
    
    platform_zero_memory(block, size);
    return block;
}

void yfree_report(u64 size, E_MEMORY_TAG tag) {
    state_ptr->stats.total_allocated -= size;
    state_ptr->stats.tagged_allocations[tag] -= size;
    state_ptr->stats.new_tagged_deallocations[tag] += size;
    state_ptr->alloc_count--;
}

b8 ymemory_get_size(void* block, u64* out_size) {
#if Y_USE_CUSTOM_MEMORY_ALLOCATOR
    b8 result = hpha_get_size(&state_ptr->allocator, block, out_size);
#else
    *out_size = 0;
    *out_alignment = 1;
    b8 result = true;
#endif
    return result;
}

void yallocate_report(u64 size, E_MEMORY_TAG tag) {
    state_ptr->stats.total_allocated += size;
    state_ptr->stats.tagged_allocations[tag] += size;
    state_ptr->stats.new_tagged_allocations[tag] += size;
    state_ptr->alloc_count++;
}

void* yreallocate(void* block, u64 old_size, u64 new_size, E_MEMORY_TAG tag) {
    return yreallocate_aligned(block, old_size, new_size, 1, tag);
}

void* yreallocate_aligned(void* block, u64 old_size, u64 new_size, u16 alignment, E_MEMORY_TAG tag) {
    void* new_block = yallocate_aligned(new_size, alignment, tag);
    if (block && new_block) {
        ycopy_memory(new_block, block, old_size);
        yfree(block, tag);
    }
    return new_block;
}

void yreallocate_report(u64 old_size, u64 new_size, E_MEMORY_TAG tag) {
    yfree_report(old_size, tag);
    yallocate_report(new_size, tag);
}


void yfree(void* block, E_MEMORY_TAG tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        PRINT_WARNING("yfree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }
    if (state_ptr) {
#if Y_USE_CUSTOM_MEMORY_ALLOCATOR
        // Track stats
        u64 size;
        // Use HPHA for freeing
        b8 result = hpha_free(&state_ptr->allocator, block, &size);
        if (!result) {
            PRINT_ERROR("Failed to free memory block in custom allocator %p", block);
        }
        yfree_report(size, tag);
#else
        yfree(block);
        b8 result = true;
#endif

        // If the free failed, it's possible this is because the allocation was made
        // before this system was started up. Since this absolutely should be an exception
        // to the rule, try freeing it on the platform level. If this fails, some other
        // brand of skulduggery is afoot, and we have bigger problems on our hands.
        if (!result) {
            // TODO: Memory alignment
            platform_free(block, false);
        }
    } else {
        // TODO: Memory alignment
        platform_free(block, false);
    }
}

void* yzero_memory(void* block, u64 size) {
    return platform_zero_memory(block, size);
}

void* ycopy_memory(void* dest, const void* source, u64 size) {
    return platform_copy_memory(dest, source, size);
}

void* yset_memory(void* dest, i32 value, u64 size) {
    return platform_set_memory(dest, value, size);
}

const char* get_unit_for_size(u64 size_bytes, f64* out_amount) {
    if (size_bytes >= GIBIBYTES(1)) {
        *out_amount = (f64)size_bytes / (GIBIBYTES(1));
        return "GiB";
    } else if (size_bytes >= MEBIBYTES(1)) {
        *out_amount = (f64)size_bytes / (MEBIBYTES(1));
        return "MiB";
    } else if (size_bytes >= KIBIBYTES(1)) {
        *out_amount = (f64)size_bytes / (KIBIBYTES(1));
        return "KiB";
    } else {
        *out_amount = (f64)size_bytes;
        return "B";
    }
}

char* get_memory_usage_str(void) {
    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        f64 amounts[3] = {1.0f, 1.0f, 1.0f};
        const char* units[3] = {
            get_unit_for_size(state_ptr->stats.tagged_allocations[i], &amounts[0]),
            get_unit_for_size(state_ptr->stats.new_tagged_allocations[i], &amounts[1]),
            get_unit_for_size(state_ptr->stats.new_tagged_deallocations[i], &amounts[2])};

        i32 length = snprintf(buffer + offset, 8000, "  %s: %-7.2f %-3s [+ %-7.2f %-3s | - %-7.2f%-3s]\n",
                              memory_tag_strings[i],
                              amounts[0], units[0], amounts[1], units[1], amounts[2], units[2]);
        offset += length;
    }
    yzero_memory(&state_ptr->stats.new_tagged_allocations, sizeof(state_ptr->stats.new_tagged_allocations));
    yzero_memory(&state_ptr->stats.new_tagged_deallocations, sizeof(state_ptr->stats.new_tagged_deallocations));
    {
// Compute total usage.
#if Y_USE_CUSTOM_MEMORY_ALLOCATOR
        // Add HPHA usage report
        u64 small_free, medium_free, large_free;
        hpha_allocator_free_space(&state_ptr->allocator, &small_free, &medium_free, &large_free);
        
        u64 small_total, medium_total, large_total;
        hpha_allocator_total_space(&state_ptr->allocator, &small_total, &medium_total, &large_total);
        u64 small_used_space = small_total - small_free;
        u64 medium_used_space = medium_total - medium_free;
        u64 large_used_space = large_total - large_free;


        u64 total_space = small_total + medium_total + large_total;
        u64 free_space = small_free + medium_free + large_free;
        u64 used_space = total_space - free_space;


#else
        u64 small_free, medium_free, large_free;
        u64 small_total, medium_total, large_total;

        small_free = 0;
        medium_free = 0;
        large_free = 0;
        small_total = 0;
        medium_total = 0;
        large_total = 0;
        u64 total_space = 0;
        // u64 free_space = 0;
        u64 used_space = 0;
#endif

        f64 used_amount = 1.0f;
        const char* used_unit = get_unit_for_size(used_space, &used_amount);

        f64 total_amount = 1.0f;
        const char* total_unit = get_unit_for_size(total_space, &total_amount);

        f64 percent_used = (f64)(used_space) / (f64)total_space * 100;

        i32 length = snprintf(buffer + offset, 8000, "Total memory usage: %.2f %s of %.2f %s (%.2f%%)\n", used_amount, used_unit, total_amount, total_unit, percent_used);
        offset += length;

        f64 small_used_amount = 1.0f;
        const char* small_used_unit = get_unit_for_size(small_used_space, &small_used_amount);

        f64 small_total_amount = 1.0f;
        const char* small_total_unit = get_unit_for_size(small_total, &small_total_amount);

        f64 small_percent_used = (f64)(small_used_space) / (f64)small_total * 100;

        i32 small_length = snprintf(buffer + offset, 8000, "Small Pool Total memory usage: %.2f %s of %.2f %s (%.2f%%)\n", small_used_amount, small_used_unit, small_total_amount, small_total_unit, small_percent_used);
        offset += small_length;

        f64 medium_used_amount = 1.0f;
        const char* medium_used_unit = get_unit_for_size(medium_used_space, &medium_used_amount);

        f64 medium_total_amount = 1.0f;
        const char* medium_total_unit = get_unit_for_size(medium_total, &medium_total_amount);

        f64 medium_percent_used = (f64)(medium_used_space) / (f64)medium_total * 100;

        i32 medium_length = snprintf(buffer + offset, 8000, "Medium Pool Total memory usage: %.2f %s of %.2f %s (%.2f%%)\n", medium_used_amount, medium_used_unit, medium_total_amount, medium_total_unit, medium_percent_used);
        offset += medium_length;

        f64 large_used_amount = 1.0f;
        const char* large_used_unit = get_unit_for_size(large_used_space, &large_used_amount);

        f64 large_total_amount = 1.0f;
        const char* large_total_unit = get_unit_for_size(large_total, &large_total_amount);

        f64 large_percent_used = (f64)(large_used_space) / (f64)large_total * 100;

        i32 large_length = snprintf(buffer + offset, 8000, "Large Pool Total memory usage: %.2f %s of %.2f %s (%.2f%%)\n", large_used_amount, large_used_unit, large_total_amount, large_total_unit, large_percent_used);
        offset += large_length;

        

    }

    
    char* out_string = string_duplicate(buffer);
    return out_string;
}

u64 get_memory_alloc_count(void) {
    if (state_ptr) {
        return state_ptr->alloc_count;
    }
    return 0;
}
