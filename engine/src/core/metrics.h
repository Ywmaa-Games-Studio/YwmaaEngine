#pragma once

#include "defines.h"

/**
 * @brief Initializes the metrics system.
 */
YAPI void metrics_init(void);

/**
 * @brief Updates metrics; should be called once per frame.
 *
 * @param frame_elapsed_time The amount of time elapsed on the previous frame.
 */
YAPI void metrics_update(f64 frame_elapsed_time);

/**
 * @brief Returns the running average frames per second (fps).
 */
YAPI f64 metrics_fps(void);

/**
 * @brief Returns the running average frametime in milliseconds.
 */
YAPI f64 metrics_frame_time(void);

/**
 * @brief Gets both the running average frames per second (fps) and frametime in milliseconds.
 *
 * @param out_fps A pointer to hold the running average frames per second (fps).
 * @param out_frame_ms A pointer to hold the running average frametime in milliseconds.
 */
YAPI void metrics_frame(f64* out_fps, f64* out_frame_ms);
