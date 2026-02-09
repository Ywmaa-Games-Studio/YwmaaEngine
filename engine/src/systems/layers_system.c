#include "layers_system.h"
#include "core/logger.h"
#include "core/ymemory.h"
#include "core/ystring.h"

#include "data_structures/stack.h"
typedef struct {
  STACK layers_stack;
  i32 top;
} LAYERS_MANAGER;

static LAYERS_MANAGER* state_ptr;

b8 layers_system_init(u64* memory_requirement, void* state, void* config) {
    *memory_requirement = sizeof(LAYERS_MANAGER);
    if (state == 0) {
        return true;
    }
    yzero_memory(state, sizeof(LAYERS_MANAGER));
    state_ptr = state;
    state_ptr->top = -1;

    // Create the layers stack.
    stack_create(&state_ptr->layers_stack, sizeof(LAYER));

    PRINT_INFO("Layers subsystem initialized.");

    return true;
}

void layers_system_shutdown(void* state) {
    while (state_ptr->top > -1) {
        layers_system_pop();
    }
    state_ptr = 0;
}

b8 layers_system_update(void* state, const struct FRAME_DATA* p_frame_data) {
    if (!state_ptr) {
        return false;
    }

    if (state_ptr->top == -1) return true;
    LAYER* layers = (LAYER*)state_ptr->layers_stack.memory;
    for (u64 i = 0; i < state_ptr->layers_stack.element_count; i++) {
        if (layers[i].update) layers[i].update(p_frame_data->delta_time);
    }
    return true;
}


u32 layers_system_push(const LAYER* layer) {
    if (state_ptr && layer) {
        state_ptr->top++;
        // Add the LAYER to the stack, then apply it.
        if (!stack_push(&state_ptr->layers_stack, (void*)layer)) {
            PRINT_ERROR("Failed to push LAYER!");
            return state_ptr->top;
        }
        if (layer->init) layer->init();
        return state_ptr->top;
    }

    return 0;
}

u32 layers_system_pop(void) {
    if (state_ptr) {
        if (state_ptr->top == -1) return 0;
        LAYER *top = layers_system_top();
        if (top->destroy) top->destroy();
        stack_pop(&state_ptr->layers_stack, (void*)top);
        state_ptr->top--;
        return state_ptr->top;
    }

    return 0;
}

LAYER* layers_system_top(void) {
    if (state_ptr) {
        LAYER* layers = (LAYER*)state_ptr->layers_stack.memory;
        return &layers[state_ptr->top];
    }
    return NULL;
}

//TODO: transition should be queued and not done immediately to avoid unexpected behaviour
b8 layers_system_transition(u32 layer_index, LAYER *target_layer) {
    if (state_ptr) {
        LAYER* layers = state_ptr->layers_stack.memory;
        if (layer_index > state_ptr->layers_stack.element_count) return false;
        if (layers[layer_index].destroy) layers[layer_index].destroy();
        ycopy_memory(&layers[layer_index], target_layer, sizeof(LAYER));
        if (layers[layer_index].init) layers[layer_index].init();
        return true;
    }
    return false;
}

