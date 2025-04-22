#include "webgpu_buffer.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "memory/freelist.h"

void webgpu_buffer_cleanup_freelist(WEBGPU_BUFFER* buffer) {
    freelist_destroy(&buffer->buffer_freelist);
    yfree_aligned(buffer->freelist_block, buffer->freelist_memory_requirement, 4, MEMORY_TAG_RENDERER);
    buffer->freelist_memory_requirement = 0;
    buffer->freelist_block = 0;
}

b8 webgpu_buffer_create(
    WEBGPU_CONTEXT* context,
    u64 size,
    WGPUBufferUsage usage,
    b8 mapped_on_create,
    WEBGPU_BUFFER* out_buffer) {
    yzero_memory(out_buffer, sizeof(WEBGPU_BUFFER));
    out_buffer->total_size = size;
    out_buffer->usage = usage;

    // Create a new freelist
    out_buffer->freelist_memory_requirement = 0;
    freelist_create(size, &out_buffer->freelist_memory_requirement, 0, 0);
    out_buffer->freelist_block = yallocate_aligned(out_buffer->freelist_memory_requirement, 4, MEMORY_TAG_RENDERER);
    freelist_create(size, &out_buffer->freelist_memory_requirement, out_buffer->freelist_block, &out_buffer->buffer_freelist);


    WGPUBufferDescriptor buffer_desc = {};
    buffer_desc.nextInChain = NULL;
    buffer_desc.label = (WGPUStringView){"Buffer", sizeof("Buffer")};
    buffer_desc.usage = usage;
    buffer_desc.size = size;
    buffer_desc.mappedAtCreation = mapped_on_create;
    out_buffer->handle = wgpuDeviceCreateBuffer(context->device, &buffer_desc);

    return true;
}

void webgpu_buffer_destroy(WEBGPU_BUFFER* buffer) {
    if (buffer->freelist_block) {
        // Make sure to destroy the freelist.
        webgpu_buffer_cleanup_freelist(buffer);
    }
    if (buffer->handle) {
        wgpuBufferDestroy(buffer->handle);
        wgpuBufferRelease(buffer->handle);
        buffer->handle = 0;
    }
    
    buffer->total_size = 0;
    buffer->usage = 0;
    buffer->is_locked = false;
}


void webgpu_buffer_load_data(WEBGPU_CONTEXT* context, WEBGPU_BUFFER* buffer, u64 offset, u64 size, const void* data) {
    wgpuQueueWriteBuffer(context->queue, buffer->handle, offset, data, size);
}

b8 webgpu_buffer_allocate(WEBGPU_BUFFER* buffer, u64 size, u64* out_offset) {
    if (!buffer || !size || !out_offset) {
        PRINT_ERROR("webgpu_buffer_allocate requires valid buffer, a nonzero size and valid pointer to hold offset.");
        return false;
    }

    return freelist_allocate_block(&buffer->buffer_freelist, size, out_offset);
}

b8 webgpu_buffer_free(WEBGPU_BUFFER* buffer, u64 size, u64 offset) {
    if (!buffer || !size || !offset) {
        PRINT_ERROR("webgpu_buffer_allocate requires valid buffer, a nonzero size and a nonzero offset.");
        return false;
    }

    return freelist_free_block(&buffer->buffer_freelist, size, offset);
}