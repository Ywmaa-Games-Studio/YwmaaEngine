#include "ring_queue.h"

#include "core/ymemory.h"
#include "core/logger.h"

b8 ring_queue_create(u32 stride, u32 capacity, void* memory, RING_QUEUE* out_queue) {
    if (!out_queue) {
        PRINT_ERROR("ring_queue_create requires a valid pointer to hold the queue.");
        return false;
    }

    out_queue->length = 0;
    out_queue->capacity = capacity;
    out_queue->stride = stride;
    out_queue->head = 0;
    out_queue->tail = -1;
    if (memory) {
        out_queue->owns_memory = false;
        out_queue->block = memory;
    } else {
        out_queue->owns_memory = true;
        out_queue->block = yallocate(capacity * stride, MEMORY_TAG_RING_QUEUE);
    }

    return true;
}

void ring_queue_destroy(RING_QUEUE* queue) {
    if (queue) {
        if (queue->owns_memory) {
            yfree(queue->block);
        }
        yzero_memory(queue, sizeof(RING_QUEUE));
    }
}

b8 ring_queue_enqueue(RING_QUEUE* queue, void* value) {
    if (queue && value) {
        if (queue->length == queue->capacity) {
            PRINT_ERROR("ring_queue_enqueue - Attempted to enqueue value in full ring queue: %p", queue);
            return false;
        }

        queue->tail = (queue->tail + 1) % queue->capacity;

        ycopy_memory(queue->block + (queue->tail * queue->stride), value, queue->stride);
        queue->length++;
        return true;
    }

    PRINT_ERROR("ring_queue_enqueue requires valid pointers to queue and value.");
    return false;
}

b8 ring_queue_dequeue(RING_QUEUE* queue, void* out_value) {
    if (queue && out_value) {
        if (queue->length == 0) {
            PRINT_ERROR("ring_queue_dequeue - Attempted to dequeue value in empty ring queue: %p", queue);
            return false;
        }

        ycopy_memory(out_value, queue->block + (queue->head * queue->stride), queue->stride);
        queue->head = (queue->head + 1) % queue->capacity;
        queue->length--;
        return true;
    }

    PRINT_ERROR("ring_queue_dequeue requires valid pointers to queue and out_value.");
    return false;
}

b8 ring_queue_peek(const RING_QUEUE* queue, void* out_value) {
    if (queue && out_value) {
        if (queue->length == 0) {
            PRINT_ERROR("ring_queue_peek - Attempted to peek value in empty ring queue: %p", queue);
            return false;
        }

        ycopy_memory(out_value, queue->block + (queue->head * queue->stride), queue->stride);
        return true;
    }

    PRINT_ERROR("ring_queue_peek requires valid pointers to queue and out_value.");
    return false;
}
