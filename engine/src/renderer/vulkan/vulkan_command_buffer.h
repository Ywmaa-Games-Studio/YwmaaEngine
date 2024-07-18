#pragma once
#include "vulkan_types.inl"

void vulkan_command_buffer_allocate(
    VULKAN_CONTEXT* context,
    VkCommandPool pool,
    b8 is_primary,
    VULKAN_COMMAND_BUFFER* out_command_buffer);

void vulkan_command_buffer_free(
    VULKAN_CONTEXT* context,
    VkCommandPool pool,
    VULKAN_COMMAND_BUFFER* command_buffer);

void vulkan_command_buffer_begin(
    VULKAN_COMMAND_BUFFER* command_buffer,
    b8 is_single_use,
    b8 is_renderpass_continue,
    b8 is_simultaneous_use);

void vulkan_command_buffer_end(VULKAN_COMMAND_BUFFER* command_buffer);

void vulkan_command_buffer_update_submitted(VULKAN_COMMAND_BUFFER* command_buffer);

void vulkan_command_buffer_reset(VULKAN_COMMAND_BUFFER* command_buffer);

/**
 * Allocates and begins recording to out_command_buffer.
 */
void vulkan_command_buffer_allocate_and_begin_single_use(
    VULKAN_CONTEXT* context,
    VkCommandPool pool,
    VULKAN_COMMAND_BUFFER* out_command_buffer);

/**
 * Ends recording, submits to and waits for queue operation and frees the provided command buffer.
 */
void vulkan_command_buffer_end_single_use(
    VULKAN_CONTEXT* context,
    VkCommandPool pool,
    VULKAN_COMMAND_BUFFER* command_buffer,
    VkQueue queue);