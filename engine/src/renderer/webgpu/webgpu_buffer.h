#pragma once

#include "webgpu_types.inl"

b8 webgpu_buffer_create(
    WEBGPU_CONTEXT* context,
    u64 size,
    WGPUBufferUsage usage,
    b8 mapped_on_create,
    WEBGPU_BUFFER* out_buffer);

void webgpu_buffer_destroy(WEBGPU_BUFFER* buffer);

void webgpu_buffer_load_data(WEBGPU_CONTEXT* context, WEBGPU_BUFFER* buffer, u64 offset, u64 size, const void* data);

/**
 * @brief Allocates space from a webgpu buffer. Provides the offset at which the
 * allocation occurred. This will be required for data copying and freeing.
 * 
 * @param buffer A pointer to the buffer from which to allocate.
 * @param size The size in bytes to be allocated.
 * @param out_offset A pointer to hold the offset in bytes from the beginning of the buffer.
 * @return True on success; otherwise false.
 */
b8 webgpu_buffer_allocate(WEBGPU_BUFFER* buffer, u64 size, u64* out_offset);

/**
 * @brief Frees space in the webgpu buffer.
 * 
 * @param buffer A pointer to the buffer to free data from.
 * @param size The size in bytes to be freed.
 * @param offset The offset in bytes from the beginning of the buffer.
 * @return True on success; otherwise false.
 */
b8 webgpu_buffer_free(WEBGPU_BUFFER* buffer, u64 size, u64 offset);
