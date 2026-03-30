#pragma once

#include "defines.h"

#include "core/frame_data.h"
#include "core/event.h"

typedef struct {
  void (*init)(void);
  void (*update)(f32);
  void (*render)(f32);
  void (*destroy)(void);
  void (*on_event)(u16, void*, EVENT_CONTEXT);
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
void layers_system_render(const struct FRAME_DATA* p_frame_data);

/**
 * @brief Updates the layers system on event triggered.
 */
void layers_system_on_event(u16 code, void* sender, EVENT_CONTEXT data);

/**
 * @brief Adds a new layer to the layers stack
 * @return returns the current top layer index
 */
u32 layers_system_push(const LAYER *layer);

/**
 * @brief Pops the top later from the layers stack
 * @return returns the current top layer index
 */
u32 layers_system_pop(void);

/**
 * @brief transitions a specific index from current layer to target_layer
 * @return returns true on success
 */
b8 layers_system_transition(u32 layer_index, LAYER *target_layer);

/**
 * @brief gets the top layer
 * @return the top layer
 */
LAYER* layers_system_top(void);
