#pragma once

#include "defines.h"

/**
 * A mutex to be used for synchronization purposes. A mutex (or
 * mutual exclusion) is used to limit access to a resource when
 * there are multiple threads of execution around that resource.
 */
typedef struct YMUTEX {
    void *internal_data;
} YMUTEX;

/**
 * Creates a mutex.
 * @param out_mutex A pointer to hold the created mutex.
 * @returns True if created successfully; otherwise false.
 */
b8 ymutex_create(YMUTEX* out_mutex);

/**
 * @brief Destroys the provided mutex.
 * 
 * @param mutex A pointer to the mutex to be destroyed.
 */
void ymutex_destroy(YMUTEX* mutex);

/**
 * Creates a mutex lock.
 * @param mutex A pointer to the mutex.
 * @returns True if locked successfully; otherwise false.
 */
b8 ymutex_lock(YMUTEX *mutex);

/**
 * Unlocks the given mutex.
 * @param mutex The mutex to unlock.
 * @returns True if unlocked successfully; otherwise false.
 */
b8 ymutex_unlock(YMUTEX *mutex);
