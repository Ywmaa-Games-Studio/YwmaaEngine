#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "renderer/renderer_types.inl"

/** @brief The configuration for the render view system. */
typedef struct RENDER_VIEW_SYSTEM_CONFIG {
    /** @brief The maximum number of views that can be registered with the system. */
    u16 max_view_count;
} RENDER_VIEW_SYSTEM_CONFIG;

/**
 * @brief Initializes the render view system. Call twice; once to obtain memory
 * requirement (where state=0) and a second time with allocated memory passed to state.
 *
 * @param memory_requirement A pointer to hold the memory requirement in bytes.
 * @param state A block of memory to be used for the state.
 * @param config Configuration for the system.
 * @return True on success; otherwise false.
 */
b8 render_view_system_init(u64* memory_requirement, void* state, RENDER_VIEW_SYSTEM_CONFIG config);

/**
 * @brief Shuts the render view system down.
 *
 * @param state The block of state memory.
 */
void render_view_system_shutdown(void* state);

/**
 * @brief Creates a new view using the provided config. The new
 * view may then be obtained via a call to render_view_system_get.
 *
 * @param config A constant pointer to the view configuration.
 * @return True on success; otherwise false.
 */
YAPI b8 render_view_system_create(const RENDER_VIEW_CONFIG* config);

/**
 * @brief Called when the owner of this view (i.e. the window) is resized.
 *
 * @param width The new width in pixels.
 * @param width The new height in pixels.
 */
YAPI void render_view_system_on_window_resize(u32 width, u32 height);

/**
 * @brief Obtains a pointer to a view with the given name.
 *
 * @param name The name of the view.
 * @return A pointer to a view if found; otherwise 0.
 */
YAPI RENDER_VIEW* render_view_system_get(const char* name);

/**
 * @brief Builds a render view packet using the provided view and meshes.
 *
 * @param view A pointer to the view to use.
 * @param frame_allocator An allocator used this frame to build a packet.
 * @param data Freeform data used to build the packet.
 * @param out_packet A pointer to hold the generated packet.
 * @return True on success; otherwise false.
 */
YAPI b8 render_view_system_build_packet(const RENDER_VIEW* view, struct LINEAR_ALLOCATOR* frame_allocator, void* data, struct RENDER_VIEW_PACKET* out_packet);

/**
 * @brief Uses the given view and packet to render the contents therein.
 *
 * @param view A pointer to the view to use.
 * @param packet A pointer to the packet whose data is to be rendered.
 * @param frame_number The current renderer frame number, typically used for data synchronization.
 * @param render_target_index The current render target index for renderers that use multiple render targets at once (i.e. Vulkan).
 * @return True on success; otherwise false.
 */
YAPI b8 render_view_system_on_render(const RENDER_VIEW* view, const RENDER_VIEW_PACKET* packet, u64 frame_number, u64 render_target_index);

YAPI void render_view_system_regenerate_render_targets(RENDER_VIEW* view);
