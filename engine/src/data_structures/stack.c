#include "stack.h"
#include "core/logger.h"
#include "core/ymemory.h"

static void stack_ensure_allocated(STACK* s, u32 count) {
    if (s->allocated < s->element_size * count) {
        void* temp = yallocate_aligned(count * s->element_size, 8, MEMORY_TAG_ARRAY);
        if (s->memory) {
            ycopy_memory(temp, s->memory, s->allocated);
            yfree(s->memory);
        }
        s->memory = temp;
        s->allocated = count * s->element_size;
    }
}

b8 stack_create(STACK* out_stack, u32 element_size) {
    if (!out_stack) {
        PRINT_ERROR("stack_create requires a pointer to a valid STACK.");
        return false;
    }

    yzero_memory(out_stack, sizeof(STACK));
    out_stack->element_size = element_size;
    out_stack->element_count = 0;
    stack_ensure_allocated(out_stack, 1);
    return true;
}

void stack_destroy(STACK* s) {
    if (s) {
        if (s->memory) {
            yfree(s->memory);
        }
        yzero_memory(s, sizeof(STACK));
    }
}

b8 stack_push(STACK* s, void* element_data) {
    if (!s) {
        PRINT_ERROR("stack_push requires a pointer to a valid STACK.");
        return false;
    }

    stack_ensure_allocated(s, s->element_count + 1);
    ycopy_memory((void*)((u64)s->memory + (s->element_count * s->element_size)), element_data, s->element_size);
    s->element_count++;
    return true;
}

b8 stack_pop(STACK* s, void* out_element_data) {
    if (!s || !out_element_data) {
        PRINT_ERROR("stack_pop requires a pointer to a valid STACK and to hold element data output.");
        return false;
    }

    if (s->element_count < 1) {
        PRINT_WARNING("Cannot pop from an empty STACK.");
        return false;
    }

    ycopy_memory(out_element_data, (void*)((u64)s->memory + ((s->element_count - 1) * s->element_size)), s->element_size);

    s->element_count--;

    return true;
}
