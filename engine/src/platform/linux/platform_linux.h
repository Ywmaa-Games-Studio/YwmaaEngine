#pragma once

#include "defines.h"

#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

#include "core/logger.h"
#include "core/event.h"
#include "input/input.h"
#include "variants/darray.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Include platform-specific Vulkan surface extensions
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR

// Include Volk
#define VOLK_IMPLEMENTATION
#include "../../../thirdparty/volk/volk.h"

// For Vulkan surface creation
#include "renderer/vulkan/vulkan_types.inl"

// For WebGPU surface creation
#include "renderer/webgpu/webgpu_types.inl"

// X11-specific platform functions
b8 platform_x11_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x, i32 y,
    i32 width, i32 height);

void platform_x11_system_shutdown(void* platform_state);

b8 platform_x11_pump_messages(void);

// X11-specific Vulkan surface creation
b8 platform_x11_create_vulkan_surface(void* context_ptr, void* state_ptr);

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

// Wayland-specific Vulkan surface creation
b8 platform_wayland_create_vulkan_surface(void* context_ptr, void* state_ptr);

// Wayland-specific WebGPU surface creation
b8 platform_wayland_create_webgpu_surface(void* context_ptr, void* state_ptr);

// Key translation
E_KEYS translate_keysym(uint32_t key);

#endif // defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID) 