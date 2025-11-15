#pragma once

#include "defines.h"

typedef struct PLATFORM_SYSTEM_CONFIG {
    /** @brief application_name The name of the application. */
    const char* application_name;
    /** @brief x The initial x position of the main window. */
    i32 x;
    /** @brief y The initial y position of the main window.*/
    i32 y;
    /** @brief width The initial width of the main window. */
    i32 width;
    /** @brief height The initial height of the main window. */
    i32 height;
} PLATFORM_SYSTEM_CONFIG;

b8 platform_system_startup(u64* memory_requirement, void* state, void* config);

void platform_system_shutdown(void* platform_state);

b8 platform_pump_messages(void);

void* platform_allocate(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, i32 value, u64 size);

void platform_console_write(const char* message, u8 color);

f64 platform_get_absolute_time(void);

// Sleep on the thread for the provided ms. This blocks the main thread.
// Should only be used for giving time back to the OS for unused update power.
// Therefore it is not exported.
void platform_sleep(u64 ms);

/**
 * @brief Obtains the number of logical processor cores.
 *
 * @return The number of logical processor cores.
 */
i32 platform_get_processor_count(void);
/**
 * @brief Obtains the required memory amount for platform-specific handle data,
 * and optionally obtains a copy of that data. Call twice, once with memory=0
 * to obtain size, then a second time where memory = allocated block.
 *
 * @param out_size A pointer to hold the memory requirement.
 * @param memory Allocated block of memory.
 */
YAPI void platform_get_handle_info(u64* out_size, void* memory);

// Linux platform layer
#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)
// Track which platform backend is active
typedef enum E_PLATFORM_BACKEND {
    PLATFORM_BACKEND_NONE,
    PLATFORM_BACKEND_WAYLAND,
    PLATFORM_BACKEND_X11
} E_PLATFORM_BACKEND;

YAPI E_PLATFORM_BACKEND platform_get_linux_active_display_protocol(void);
#endif
