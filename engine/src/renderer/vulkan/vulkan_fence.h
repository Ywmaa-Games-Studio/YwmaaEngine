#pragma once

#include "vulkan_types.inl"

void vulkan_fence_create(
    VULKAN_CONTEXT* context,
    b8 create_signaled,
    VULKAN_FENCE* out_fence);

void vulkan_fence_destroy(VULKAN_CONTEXT* context, VULKAN_FENCE* fence);

b8 vulkan_fence_wait(VULKAN_CONTEXT* context, VULKAN_FENCE* fence, u64 timeout_ns);

void vulkan_fence_reset(VULKAN_CONTEXT* context, VULKAN_FENCE* fence);
