#pragma once

#include "defines.h"

/** @brief The configuration for the memory system. */
typedef struct MEMORY_SYSTEM_CONFIG {
    /** @brief The total memory size in byes used by the internal allocator for this system. */
    u64 total_alloc_size;
} MEMORY_SYSTEM_CONFIG;

typedef struct FRAME_ALLOCATOR_INT {
    void* (*allocate)(u64 size);
    void (*free)(void* block, u64 size);
    void (*free_all)(void);
} FRAME_ALLOCATOR_INT;


typedef enum E_MEMORY_TAG {
    // For temporary use. Should be assigned one of the below or have a new tag created.
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,

    MEMORY_TAG_MAX_TAGS
} E_MEMORY_TAG;

YAPI b8 memory_system_init(MEMORY_SYSTEM_CONFIG config);
YAPI void memory_system_shutdown();

YAPI void* yallocate(u64 size, E_MEMORY_TAG tag);

YAPI void* yallocate_aligned(u64 size, u16 alignment, E_MEMORY_TAG tag);

YAPI void yallocate_report(u64 size, E_MEMORY_TAG tag);

YAPI void* yreallocate(void* block, u64 old_size, u64 new_size, E_MEMORY_TAG tag);

YAPI void* yreallocate_aligned(void* block, u64 old_size, u64 new_size, u16 alignment, E_MEMORY_TAG tag);

YAPI void yreallocate_report(u64 old_size, u64 new_size, E_MEMORY_TAG tag);

YAPI void yfree(void* block, u64 size, E_MEMORY_TAG tag);

YAPI void yfree_aligned(void* block, u64 size, u16 alignment, E_MEMORY_TAG tag);

YAPI void kfree_report(u64 size, E_MEMORY_TAG tag);

YAPI b8 ymemory_get_size_alignment(void* block, u64* out_size, u16* out_alignment);

YAPI void* yzero_memory(void* block, u64 size);

YAPI void* ycopy_memory(void* dest, const void* source, u64 size);

YAPI void* yset_memory(void* dest, i32 value, u64 size);

YAPI char* get_memory_usage_str(void);

YAPI u64 get_memory_alloc_count();