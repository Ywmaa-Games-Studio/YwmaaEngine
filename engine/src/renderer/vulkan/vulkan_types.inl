#pragma once

#include "defines.h"

#include <vulkan/vulkan.h>


typedef struct VULKAN_CONTEXT {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
} VULKAN_CONTEXT;