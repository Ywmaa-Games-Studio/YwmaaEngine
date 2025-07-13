#include "job_system.h"

#include "core/ythread.h"
#include "core/ymutex.h"
#include "core/ymemory.h"
#include "core/logger.h"
#include "data_structures/ring_queue.h"

typedef struct JOB_THREAD {
    u8 index;
    YTHREAD thread;
    JOB_INFO info;
    // A mutex to guard access to this thread's info.
    YMUTEX info_mutex;

    // The types of jobs this thread can handle.
    u32 type_mask;
} JOB_THREAD;

typedef struct JOB_RESULT_ENTRY {
    u16 id;
    PFN_JOB_ON_COMPLETE callback;
    u32 param_size;
    void* params;
} JOB_RESULT_ENTRY;

// The max number of job results that can be stored at once.
#define MAX_JOB_RESULTS 512

typedef struct JOB_SYSTEM_STATE {
    b8 running;
    u8 thread_count;
    JOB_THREAD job_threads[32];

    RING_QUEUE low_priority_queue;
    RING_QUEUE normal_priority_queue;
    RING_QUEUE high_priority_queue;

    // Mutexes for each queue, since a job could be kicked off from another job (thread).
    YMUTEX low_pri_queue_mutex;
    YMUTEX normal_pri_queue_mutex;
    YMUTEX high_pri_queue_mutex;

    JOB_RESULT_ENTRY pending_results[MAX_JOB_RESULTS];
    YMUTEX result_mutex;
    // A mutex for the result array
} JOB_SYSTEM_STATE;

static JOB_SYSTEM_STATE* state_ptr;

void store_result(PFN_JOB_ON_COMPLETE callback, u32 param_size, void* params) {
    // Create the new entry.
    JOB_RESULT_ENTRY entry;
    entry.id = INVALID_ID_U16;
    entry.param_size = param_size;
    entry.callback = callback;
    if (entry.param_size > 0) {
        // Take a copy, as the job is destroyed after this.
        entry.params = yallocate(param_size, MEMORY_TAG_JOB);
        ycopy_memory(entry.params, params, param_size);
    } else {
        entry.params = 0;
    }

    // Lock, find a free space, store, unlock.
    if (!ymutex_lock(&state_ptr->result_mutex)) {
        PRINT_ERROR("Failed to obtain mutex lock for storing a result! Result storage may be corrupted.");
    }
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        if (state_ptr->pending_results[i].id == INVALID_ID_U16) {
            state_ptr->pending_results[i] = entry;
            state_ptr->pending_results[i].id = i;
            break;
        }
    }
    if (!ymutex_unlock(&state_ptr->result_mutex)) {
        PRINT_ERROR("Failed to release mutex lock for result storage, storage may be corrupted.");
    }
}

u32 job_thread_run(void* params) {
    u32 index = *(u32*)params;
    JOB_THREAD* thread = &state_ptr->job_threads[index];
    u64 thread_id = thread->thread.thread_id;
    PRINT_TRACE("Starting job thread #%i (id=%#x, type=%#x).", thread->index, thread_id, thread->type_mask);

    // A mutex to lock info for this thread.
    if (!ymutex_create(&thread->info_mutex)) {
        PRINT_ERROR("Failed to create job thread mutex! Aborting thread.");
        return 0;
    }

    // Run forever, waiting for jobs.
    while (true) {
        if (!state_ptr || !state_ptr->running || !thread) {
            break;
        }

        // Lock and grab a copy of the info
        if (!ymutex_lock(&thread->info_mutex)) {
            PRINT_ERROR("Failed to obtain lock on job thread mutex!");
        }
        JOB_INFO info = thread->info;
        if (!ymutex_unlock(&thread->info_mutex)) {
            PRINT_ERROR("Failed to release lock on job thread mutex!");
        }

        if (info.entry_point) {
            b8 result = info.entry_point(info.param_data, info.result_data);

            // Store the result to be executed on the main thread later.
            // Note that store_result takes a copy of the result_data
            // so it does not have to be held onto by this thread any longer.
            if (result && info.on_success) {
                store_result(info.on_success, info.result_data_size, info.result_data);
            } else if (!result && info.on_fail) {
                store_result(info.on_fail, info.result_data_size, info.result_data);
            }

            // Clear the param data and result data.
            if (info.param_data) {
                yfree(info.param_data);
            }
            if (info.result_data) {
                yfree(info.result_data);
            }

            // Lock and reset the thread's info object
            if (!ymutex_lock(&thread->info_mutex)) {
                PRINT_ERROR("Failed to obtain lock on job thread mutex!");
            }
            yzero_memory(&thread->info, sizeof(JOB_INFO));
            if (!ymutex_unlock(&thread->info_mutex)) {
                PRINT_ERROR("Failed to release lock on job thread mutex!");
            }
        }

        if (state_ptr->running) {
            // TODO: Should probably find a better way to do this, such as sleeping until
            // a request comes through for a new job.
            ythread_sleep(&thread->thread, 10);
        } else {
            break;
        }
    }

    // Destroy the mutex for this thread.
    ymutex_destroy(&thread->info_mutex);
    return 1;
}

b8 job_system_init(u64* job_system_memory_requirement, void* state, u8 job_thread_count, u32 type_masks[]) {
    *job_system_memory_requirement = sizeof(JOB_SYSTEM_STATE);
    if (state == 0) {
        return true;
    }

    yzero_memory(state, sizeof(JOB_SYSTEM_STATE));

    state_ptr = state;
    state_ptr->running = true;

    ring_queue_create(sizeof(JOB_INFO), 1024, 0, &state_ptr->low_priority_queue);
    ring_queue_create(sizeof(JOB_INFO), 1024, 0, &state_ptr->normal_priority_queue);
    ring_queue_create(sizeof(JOB_INFO), 1024, 0, &state_ptr->high_priority_queue);
    state_ptr->thread_count = job_thread_count;

    // Invalidate all result slots
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        state_ptr->pending_results[i].id = INVALID_ID_U16;
    }

    PRINT_DEBUG("Main thread id is: %#x", get_thread_id());

    PRINT_DEBUG("Spawning %i job threads.", state_ptr->thread_count);

    for (u8 i = 0; i < state_ptr->thread_count; ++i) {
        state_ptr->job_threads[i].index = i;
        state_ptr->job_threads[i].type_mask = type_masks[i];
        if (!ythread_create(job_thread_run, &state_ptr->job_threads[i].index, false, &state_ptr->job_threads[i].thread)) {
            PRINT_ERROR("OS Error in creating job thread. Application cannot continue.");
            return false;
        }
        yzero_memory(&state_ptr->job_threads[i].info, sizeof(JOB_INFO));
    }

    // Create needed mutexes
    if (!ymutex_create(&state_ptr->result_mutex)) {
        PRINT_ERROR("Failed to create result mutex!.");
        return false;
    }
    if (!ymutex_create(&state_ptr->low_pri_queue_mutex)) {
        PRINT_ERROR("Failed to create low priority queue mutex!.");
        return false;
    }
    if (!ymutex_create(&state_ptr->normal_pri_queue_mutex)) {
        PRINT_ERROR("Failed to create normal priority queue mutex!.");
        return false;
    }
    if (!ymutex_create(&state_ptr->high_pri_queue_mutex)) {
        PRINT_ERROR("Failed to create high priority queue mutex!.");
        return false;
    }

    return true;
}

void job_system_shutdown(void* state) {
    if (state_ptr) {
        state_ptr->running = false;

        u64 thread_count = state_ptr->thread_count;

        // Check for a free thread first.
        for (u8 i = 0; i < thread_count; ++i) {
            ythread_destroy(&state_ptr->job_threads[i].thread);
        }
        ring_queue_destroy(&state_ptr->low_priority_queue);
        ring_queue_destroy(&state_ptr->normal_priority_queue);
        ring_queue_destroy(&state_ptr->high_priority_queue);

        // Destroy mutexes
        ymutex_destroy(&state_ptr->result_mutex);
        ymutex_destroy(&state_ptr->low_pri_queue_mutex);
        ymutex_destroy(&state_ptr->normal_pri_queue_mutex);
        ymutex_destroy(&state_ptr->high_pri_queue_mutex);

        state_ptr = 0;
    }
}

void process_queue(RING_QUEUE* queue, YMUTEX* queue_mutex) {
    u64 thread_count = state_ptr->thread_count;

    // Check for a free thread first.
    while (queue->length > 0) {
        JOB_INFO info;
        if (!ring_queue_peek(queue, &info)) {
            break;
        }

        b8 thread_found = false;
        for (u8 i = 0; i < thread_count; ++i) {
            JOB_THREAD* thread = &state_ptr->job_threads[i];
            if ((thread->type_mask & info.type) == 0) {
                continue;
            }

            // Check that the job thread can handle the job type.
            if (!ymutex_lock(&thread->info_mutex)) {
                PRINT_ERROR("Failed to obtain lock on job thread mutex!");
            }
            if (!thread->info.entry_point) {
                // Make sure to remove the entry from the queue.
                if (!ymutex_lock(queue_mutex)) {
                    PRINT_ERROR("Failed to obtain lock on queue mutex!");
                }
                ring_queue_dequeue(queue, &info);
                if (!ymutex_unlock(queue_mutex)) {
                    PRINT_ERROR("Failed to release lock on queue mutex!");
                }
                thread->info = info;
                PRINT_TRACE("Assigning job to thread: %u", thread->index);
                thread_found = true;
            }
            if (!ymutex_unlock(&thread->info_mutex)) {
                PRINT_ERROR("Failed to release lock on job thread mutex!");
            }

            // Break after unlocking if an available thread was found.
            if (thread_found) {
                break;
            }
        }

        // This means all of the threads are currently handling a job,
        // So wait until the next update and try again.
        if (!thread_found) {
            break;
        }
    }
}

void job_system_update(void) {
    if (!state_ptr || !state_ptr->running) {
        return;
    }

    process_queue(&state_ptr->high_priority_queue, &state_ptr->high_pri_queue_mutex);
    process_queue(&state_ptr->normal_priority_queue, &state_ptr->normal_pri_queue_mutex);
    process_queue(&state_ptr->low_priority_queue, &state_ptr->low_pri_queue_mutex);

    // Process pending results.
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        // Lock and take a copy, unlock.
        if (!ymutex_lock(&state_ptr->result_mutex)) {
            PRINT_ERROR("Failed to obtain lock on result mutex!");
        }
        JOB_RESULT_ENTRY entry = state_ptr->pending_results[i];
        if (!ymutex_unlock(&state_ptr->result_mutex)) {
            PRINT_ERROR("Failed to release lock on result mutex!");
        }

        if (entry.id != INVALID_ID_U16) {
            // Execute the callback.
            entry.callback(entry.params);

            if (entry.params) {
                yfree(entry.params);
            }

            // Lock actual entry, invalidate and clear it
            if (!ymutex_lock(&state_ptr->result_mutex)) {
                PRINT_ERROR("Failed to obtain lock on result mutex!");
            }
            yzero_memory(&state_ptr->pending_results[i], sizeof(JOB_RESULT_ENTRY));
            state_ptr->pending_results[i].id = INVALID_ID_U16;
            if (!ymutex_unlock(&state_ptr->result_mutex)) {
                PRINT_ERROR("Failed to release lock on result mutex!");
            }
        }
    }
}

void job_system_submit(JOB_INFO info) {
    u64 thread_count = state_ptr->thread_count;
    RING_QUEUE* queue = &state_ptr->normal_priority_queue;
    YMUTEX* queue_mutex = &state_ptr->normal_pri_queue_mutex;

    // If the job is high priority, try to kick it off immediately.
    if (info.priority == JOB_PRIORITY_HIGH) {
        queue = &state_ptr->high_priority_queue;
        queue_mutex = &state_ptr->high_pri_queue_mutex;

        // Check for a free thread that supports the job type first.
        for (u8 i = 0; i < thread_count; ++i) {
            JOB_THREAD* thread = &state_ptr->job_threads[i];
            if (state_ptr->job_threads[i].type_mask & info.type) {
                b8 found = false;
                if (!ymutex_lock(&thread->info_mutex)) {
                    PRINT_ERROR("Failed to obtain lock on job thread mutex!");
                }
                if (!state_ptr->job_threads[i].info.entry_point) {
                    PRINT_TRACE("Job immediately submitted on thread %i", state_ptr->job_threads[i].index);
                    state_ptr->job_threads[i].info = info;
                    found = true;
                }
                if (!ymutex_unlock(&thread->info_mutex)) {
                    PRINT_ERROR("Failed to release lock on job thread mutex!");
                }
                if (found) {
                    return;
                }
            }
        }
    }

    // If this point is reached, all threads are busy (if high) or it can wait a frame.
    // Add to the queue and try again next cycle.
    if (info.priority == JOB_PRIORITY_LOW) {
        queue = &state_ptr->low_priority_queue;
        queue_mutex = &state_ptr->low_pri_queue_mutex;
    }

    // NOTE: Locking here in case the job is submitted from another job/thread.
    if (!ymutex_lock(queue_mutex)) {
        PRINT_ERROR("Failed to obtain lock on queue mutex!");
    }
    ring_queue_enqueue(queue, &info);
    if (!ymutex_unlock(queue_mutex)) {
        PRINT_ERROR("Failed to release lock on queue mutex!");
    }
    PRINT_TRACE("Job queued.");
}

JOB_INFO job_create(PFN_JOB_START entry_point, PFN_JOB_ON_COMPLETE on_success, PFN_JOB_ON_COMPLETE on_fail, void* param_data, u32 param_data_size, u32 result_data_size) {
    return job_create_priority(entry_point, on_success, on_fail, param_data, param_data_size, result_data_size, JOB_TYPE_GENERAL, JOB_PRIORITY_NORMAL);
}

JOB_INFO job_create_type(PFN_JOB_START entry_point, PFN_JOB_ON_COMPLETE on_success, PFN_JOB_ON_COMPLETE on_fail, void* param_data, u32 param_data_size, u32 result_data_size, E_JOB_TYPE type) {
    return job_create_priority(entry_point, on_success, on_fail, param_data, param_data_size, result_data_size, type, JOB_PRIORITY_NORMAL);
}

JOB_INFO job_create_priority(PFN_JOB_START entry_point, PFN_JOB_ON_COMPLETE on_success, PFN_JOB_ON_COMPLETE on_fail, void* param_data, u32 param_data_size, u32 result_data_size, E_JOB_TYPE type, JOB_PRIORITY priority) {
    JOB_INFO job;
    job.entry_point = entry_point;
    job.on_success = on_success;
    job.on_fail = on_fail;
    job.type = type;
    job.priority = priority;

    job.param_data_size = param_data_size;
    if (param_data_size) {
        job.param_data = yallocate(param_data_size, MEMORY_TAG_JOB);
        ycopy_memory(job.param_data, param_data, param_data_size);
    } else {
        job.param_data = 0;
    }

    job.result_data_size = result_data_size;
    if (result_data_size) {
        job.result_data = yallocate(result_data_size, MEMORY_TAG_JOB);
    } else {
        job.result_data = 0;
    }

    return job;
}
