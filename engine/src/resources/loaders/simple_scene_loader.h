/**
 * @file simple_scene_loader.h
 * @brief Loader for simple scene files.
 * @version 1.0
 * 
 */

#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a simple scene resource loader.
 * 
 * @return The newly created resource loader.
 */
RESOURCE_LOADER simple_scene_resource_loader_create(void);
