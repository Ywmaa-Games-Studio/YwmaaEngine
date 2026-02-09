#pragma once

#include "defines.h"

#include "core/frame_data.h"

typedef struct {
  void (*init)(void);
  void (*update)(f32);
  void (*draw)(f32);
  void (*destroy)(void);
} LAYER;

b8 layers_system_init(u64* memory_requirement, void* state, void* config);
void layers_system_shutdown(void* state);

/**
 * @brief Updates the layers system. Should happen once an update cycle.
 */
b8 layers_system_update(void* state, const struct FRAME_DATA* p_frame_data);

/**
 * @brief Updates the layers system before render. Should happen once a render call.
 */
//void layers_system_render(void* state, const struct FRAME_DATA* p_frame_data);

u32 layers_system_push(const LAYER *layer);
u32 layers_system_pop(void);

b8 layers_system_transition(u32 layer_index, LAYER *target_layer);

LAYER* layers_system_top(void);

