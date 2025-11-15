#pragma once

#include <defines.h>

struct PLATFORM_STATE;
struct VULKAN_CONTEXT;

b8 platform_create_vulkan_surface(struct VULKAN_CONTEXT* context);

/**
 * Appends the names of required extensions for this platform to
 * the names_darray, which should be created and passed in.
 */
void platform_get_required_extension_names(const char*** names_darray);
