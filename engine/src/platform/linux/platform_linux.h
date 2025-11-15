#pragma once

#include "defines.h"

#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

#include "core/logger.h"
#include "core/event.h"
#include "input/input.h"
#include "core/ythread.h"
#include "core/ymutex.h"
#include "data_structures/darray.h"
#include "core/ymemory.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <errno.h>        // For error reporting
#include <sys/sysinfo.h>  // Processor info


// X11-specific platform functions
b8 platform_x11_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x, i32 y,
    i32 width, i32 height);

void platform_x11_system_shutdown(void* platform_state);

b8 platform_x11_pump_messages(void);

void platform_x11_get_handle_info(u64 *out_size, void *memory);

// X11-specific WebGPU surface creation
b8 platform_x11_create_webgpu_surface(void* context_ptr, void* state_ptr);

// Key translation
E_KEYS translate_keycode(u32 x_keycode);

// Wayland-specific platform functions
b8 platform_wayland_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x, i32 y,
    i32 width, i32 height);

void platform_wayland_system_shutdown(void* platform_state);

b8 platform_wayland_pump_messages(void);


void platform_wayland_get_handle_info(u64 *out_size, void *memory);

// Key translation
E_KEYS translate_keysym(u32 key);

#endif // defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID) 
