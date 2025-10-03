#pragma once

#include "defines.h"

typedef struct STACK {
    u32 element_size;
    u32 element_count;
    u32 allocated;
    void* memory;
} STACK;

YAPI b8 stack_create(STACK* out_stack, u32 element_size);
YAPI void stack_destroy(STACK* s);

YAPI b8 stack_push(STACK* s, void* element_data);
YAPI b8 stack_pop(STACK* s, void* out_element_data);
