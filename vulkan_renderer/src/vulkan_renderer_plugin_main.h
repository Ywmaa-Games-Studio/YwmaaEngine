#pragma once

#include <renderer/renderer_types.inl>

/**
 * @brief Creates a new renderer plugin of the given type.
 * 
 * @param out_renderer_backend A pointer to hold the newly-created renderer plugin.

 * @return True if successful; otherwise false.
 */
YAPI b8 vulkan_renderer_plugin_create(RENDERER_PLUGIN* out_plugin);
/**
 * @brief Destroys the given renderer backend.
 * 
 * @param renderer_backend A pointer to the plugin to be destroyed.
 */
YAPI void vulkan_renderer_plugin_destroy(RENDERER_PLUGIN* plugin);
