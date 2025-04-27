#include "core/ymemory.h"
#include "core/logger.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdalign.h>

#define NUM_ITERATIONS 10000
#define NUM_ALLOCATIONS 10000
#define MAX_ALLOC_SIZE 1024
#define ALIGNMENT_TEST_CASES 7
static const u16 alignments[] = {1, 4, 8, 16, 32, 64, 256}; // Test various alignments

typedef struct {
    void* ptr;
    u64 size;
    u16 alignment;
} Allocation;

void warmup_cache(void) {
    // Warm up cache with dummy allocations
    for (int i = 0; i < 1000; i++) {
        void* p = malloc(16);
        free(p);
    }
}

void benchmark_malloc(u16 alignment) {
    Allocation allocs[NUM_ALLOCATIONS] = {0};
    clock_t start, end;
    double total_time = 0;

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // Allocation phase
        start = clock();
        for (int i = 0; i < NUM_ALLOCATIONS; i++) {
            allocs[i].size = (rand() % MAX_ALLOC_SIZE) + 1;
            allocs[i].alignment = alignment;
            
            #if defined(_WIN32)
            allocs[i].ptr = _aligned_malloc(allocs[i].size, alignment);
            #else
            if (posix_memalign(&allocs[i].ptr, alignment, allocs[i].size) != 0) {
                allocs[i].ptr = NULL;
            }
            #endif
        }
        end = clock();
        total_time += ((double)(end - start)) / CLOCKS_PER_SEC;

        // Free phase
        start = clock();
        for (int i = 0; i < NUM_ALLOCATIONS; i++) {
            #if defined(_WIN32)
            _aligned_free(allocs[i].ptr);
            #else
            free(allocs[i].ptr);
            #endif
        }
        end = clock();
        total_time += ((double)(end - start)) / CLOCKS_PER_SEC;
    }

    printf("malloc(align=%3hu): %.4f sec\n", alignment, total_time);
}

void benchmark_yallocate(u16 alignment) {
    Allocation allocs[NUM_ALLOCATIONS] = {0};
    clock_t start, end;
    double total_time = 0;

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // Allocation phase
        start = clock();
        for (int i = 0; i < NUM_ALLOCATIONS; i++) {
            allocs[i].size = (rand() % MAX_ALLOC_SIZE) + 1;
            allocs[i].alignment = alignment;
            //PRINT_DEBUG("Allocating %llu bytes with alignment %hu", allocs[i].size, allocs[i].alignment);
            allocs[i].ptr = yallocate_aligned(allocs[i].size, alignment, MEMORY_TAG_APPLICATION);
        }
        end = clock();
        total_time += ((double)(end - start)) / CLOCKS_PER_SEC;

        // Free phase
        start = clock();
        for (int i = 0; i < NUM_ALLOCATIONS; i++) {
            yfree(allocs[i].ptr, MEMORY_TAG_APPLICATION);
        }
        end = clock();
        total_time += ((double)(end - start)) / CLOCKS_PER_SEC;
    }

    printf("yallocate(align=%3hu): %.4f sec", alignment, total_time);

    // Print allocation sizes
/*     char* mem_usage = get_memory_usage_str();
    PRINT_INFO(mem_usage); */

}

void run_alignment_tests(void) {
    printf("\n=== Alignment Validation ===\n");
    for (int i = 0; i < ALIGNMENT_TEST_CASES; i++) {
        u16 alignment = alignments[i];
        void* ptr = yallocate_aligned(128, alignment, MEMORY_TAG_APPLICATION);
        b8 is_aligned = ((u64)ptr % alignment) == 0;
        printf("Requested: %3hu bytes, Actual: %s\n", 
               alignment, 
               is_aligned ? "Aligned" : "Unaligned");
        yfree(ptr, MEMORY_TAG_APPLICATION);
    }
}

void run_benchmarks(void) {
    MEMORY_SYSTEM_CONFIG config = {
        .total_alloc_size = MEBIBYTES(1024)
    };
    memory_system_init(config);
    
    warmup_cache();
    printf("=== Allocation Benchmark ===\n");
    
    for (int i = 0; i < ALIGNMENT_TEST_CASES; i++) {
        benchmark_malloc(alignments[i]);
        benchmark_yallocate(alignments[i]);
        printf("\n");
    }
    memory_system_shutdown();
}
