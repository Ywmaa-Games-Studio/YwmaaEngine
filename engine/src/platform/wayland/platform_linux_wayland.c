#include "../platform.h"
// AI helped me creating this wayland backend and I tweaked stuff and removed useless stuff -for now- and still I should probably later do some rewriting and reorganizing of this implementation
// Linux platform layer - Wayland implementation
#if defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED) && !defined(YPLATFORM_ANDROID)

#include "core/logger.h"
#include "core/event.h"
#include "input/input.h"
#include "variants/darray.h"

#include <wayland-client.h>
#include <wayland-cursor.h>
#include "xdg-shell.h"

#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>  // For printf
#include <string.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#else
#include <unistd.h>  // usleep
#endif

// For Vulkan surface creation
#define VK_USE_PLATFORM_WAYLAND_KHR
#define VOLK_IMPLEMENTATION
#include "../thirdparty/volk/volk.h"
#include "renderer/vulkan/vulkan_types.inl"

// For WebGPU surface creation
#include "renderer/webgpu/webgpu_types.inl"

typedef struct PLATFORM_STATE {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct xdg_wm_base* xdg_wm_base;
    struct xdg_surface* xdg_surface;
    struct xdg_toplevel* xdg_toplevel;
    struct wl_seat* seat;
    struct wl_pointer* pointer;
    struct wl_keyboard* keyboard;
    struct wl_shm* shm;
    struct wl_cursor_theme* cursor_theme;
    struct wl_cursor* default_cursor;
    struct wl_surface* cursor_surface;
    
    i32 width;
    i32 height;
    b8 closed;
    b8 resized;
    b8 has_focus;
    
    VkSurfaceKHR vulkan_surface;
    WGPUSurface webgpu_surface;
} PLATFORM_STATE;

static PLATFORM_STATE* state_ptr;

// Wayland protocol handlers
static void xdg_surface_handle_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    xdg_surface_ack_configure(xdg_surface, serial);
    
    if (state->resized) {
        EVENT_CONTEXT context;
        context.data.u16[0] = (u16)state->width;
        context.data.u16[1] = (u16)state->height;
        event_fire(EVENT_CODE_RESIZED, 0, context);
        state->resized = false;
    }
}

static void xdg_toplevel_handle_configure(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    
    if (width > 0 && height > 0) {
        if (state->width != width || state->height != height) {
            state->width = width;
            state->height = height;
            state->resized = true;
        }
    }
    
    // Check focus state
    uint32_t* state_value;
    state->has_focus = false;
    wl_array_for_each(state_value, states) {
        if (*state_value == XDG_TOPLEVEL_STATE_ACTIVATED) {
            state->has_focus = true;
            break;
        }
    }
}

static void xdg_toplevel_handle_close(void* data, struct xdg_toplevel* toplevel) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    state->closed = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_handle_configure,
};

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close = xdg_toplevel_handle_close,
};

static void wm_base_ping(void* data, struct xdg_wm_base* wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = wm_base_ping,
};

// Keyboard input translation
E_KEYS translate_keysym(uint32_t key) {
    // Map Linux input keycodes to engine keycodes
    switch (key) {
        case 14: return KEY_BACKSPACE;
        case 28: return KEY_ENTER;
        case 15: return KEY_TAB;
        case 164: return KEY_PAUSE;
        case 58: return KEY_CAPITAL;
        case 1: return KEY_ESCAPE;
        case 57: return KEY_SPACE;
        case 104: return KEY_PRIOR;
        case 109: return KEY_NEXT;
        case 107: return KEY_END;
        case 102: return KEY_HOME;
        case 105: return KEY_LEFT;
        case 103: return KEY_UP;
        case 106: return KEY_RIGHT;
        case 108: return KEY_DOWN;
        case 634: return KEY_PRINT;
        case 110: return KEY_INSERT;
        case 111: return KEY_DELETE;
        
        case 2: return KEY_NUMPAD1;
        case 3: return KEY_NUMPAD2;
        case 4: return KEY_NUMPAD3;
        case 5: return KEY_NUMPAD4;
        case 6: return KEY_NUMPAD5;
        case 7: return KEY_NUMPAD6;
        case 8: return KEY_NUMPAD7;
        case 9: return KEY_NUMPAD8;
        case 10: return KEY_NUMPAD9;
        case 11: return KEY_NUMPAD0;
        
        case 30: return KEY_A;
        case 48: return KEY_B;
        case 46: return KEY_C;
        case 32: return KEY_D;
        case 18: return KEY_E;
        case 33: return KEY_F;
        case 34: return KEY_G;
        case 35: return KEY_H;
        case 23: return KEY_I;
        case 36: return KEY_J;
        case 37: return KEY_K;
        case 38: return KEY_L;
        case 50: return KEY_M;
        case 49: return KEY_N;
        case 24: return KEY_O;
        case 25: return KEY_P;
        case 16: return KEY_Q;
        case 19: return KEY_R;
        case 31: return KEY_S;
        case 20: return KEY_T;
        case 22: return KEY_U;
        case 47: return KEY_V;
        case 17: return KEY_W;
        case 45: return KEY_X;
        case 21: return KEY_Y;
        case 44: return KEY_Z;
        
        case 59: return KEY_F1;
        case 60: return KEY_F2;
        case 61: return KEY_F3;
        case 62: return KEY_F4;
        case 63: return KEY_F5;
        case 64: return KEY_F6;
        case 65: return KEY_F7;
        case 66: return KEY_F8;
        case 67: return KEY_F9;
        case 68: return KEY_F10;
        case 87: return KEY_F11;
        case 88: return KEY_F12;
        
        case 69: return KEY_NUMLOCK;
        //case KEY_SCROLLLOCK: return KEY_SCROLL;
        
        case 42: return KEY_LSHIFT;
        case 54: return KEY_RSHIFT;
        case 29: return KEY_LCONTROL;
        case 97: return KEY_RCONTROL;
        case 56: return KEY_LALT;
        case 100: return KEY_RALT;
        case 125: return KEY_LWIN;
        //case 125: return KEY_RWIN;
        
        case 39: return KEY_SEMICOLON;
        case 13: return KEY_PLUS;
        case 51: return KEY_COMMA;
        case 12: return KEY_MINUS;
        case 52: return KEY_PERIOD;
        case 53: return KEY_SLASH;
        case 41: return KEY_GRAVE;
        
        default: return 0;
    }
}

// Keyboard input handling
static void keyboard_handle_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size) {
    // Just close the fd if we're not interested in the keymap
    //close(fd);
}

static void keyboard_handle_enter(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    state->has_focus = true;
}

static void keyboard_handle_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    state->has_focus = false;
}

static void keyboard_handle_key(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    // Convert keycode to our key format and forward to input system
    PRINT_INFO("%i\n", key);
    E_KEYS translated_key = translate_keysym(key);
    input_process_key(translated_key, state == WL_KEYBOARD_KEY_STATE_PRESSED);
}

static void keyboard_handle_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    // Handle modifiers if needed
}

// Add this callback function for keyboard repeat info
static void keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                       int32_t rate, int32_t delay) {
    // This event provides information about key repeating:
    // - rate: characters per second, or 0 for no repeat
    // - delay: delay in ms before repeating starts
    PRINT_DEBUG("Keyboard repeat info - rate: %d, delay: %d", rate, delay);
}

// Update your keyboard_listener structure to include all events
static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_handle_keymap,
    .enter = keyboard_handle_enter,
    .leave = keyboard_handle_leave,
    .key = keyboard_handle_key,
    .modifiers = keyboard_handle_modifiers,
    .repeat_info = keyboard_handle_repeat_info,
};

// Pointer (mouse) input handling
static void pointer_handle_enter(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;

    // PRINT_DEBUG("Mouse entered");
    
    // Set cursor
    if (state->cursor_theme && state->default_cursor) {
        struct wl_cursor_image* image = state->default_cursor->images[0];
        struct wl_buffer* buffer = wl_cursor_image_get_buffer(image);
        
        wl_pointer_set_cursor(pointer, serial, state->cursor_surface, 
                             image->hotspot_x, image->hotspot_y);
        wl_surface_attach(state->cursor_surface, buffer, 0, 0);
        wl_surface_commit(state->cursor_surface);
    }
    
    // Process mouse position
    input_process_mouse_move(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
}

static void pointer_handle_leave(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface) {
    // Handle mouse leaving window if needed
    // PRINT_DEBUG("Mouse left");
}

static void pointer_handle_motion(void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    // Process mouse position
    input_process_mouse_move(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
}

static void pointer_handle_button(void* data, struct wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    // Map Wayland button codes to our engine codes
    E_BUTTONS mapped_button;
    switch (button) {
        case 0x110:
            mapped_button = BUTTON_LEFT;
            break;
        case 0x111:
            mapped_button = BUTTON_RIGHT;
            break;
        case 0x112:
            mapped_button = BUTTON_MIDDLE;
            break;
        default:
            mapped_button = BUTTON_MAX_BUTTONS;
    }
    
    if (mapped_button != BUTTON_MAX_BUTTONS) {
        input_process_button(mapped_button, state == WL_POINTER_BUTTON_STATE_PRESSED);
    }
}

// Override the console output functions to ensure printf is properly included
void platform_console_write(const char* message, u8 color) {
    // ERROR,WARN,INFO,DEBUG
    const char* color_strings[] = {"1;31", "1;33", "1;32", "1;34"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

void platform_console_write_error(const char* message, u8 color) {
    // ERROR,WARN,INFO,DEBUG
    const char* color_strings[] = {"1;31", "1;33", "1;32", "1;34"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

static void pointer_handle_axis(void* data, struct wl_pointer* wl_pointer,
                          uint32_t time, uint32_t axis, wl_fixed_t value) {
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        i32 z_delta = value;
        if (z_delta != 0) {
            // Flatten the input to an OS-independent (-1, 1)
            z_delta = (z_delta < 0) ? 1 : -1;
            input_process_mouse_wheel(z_delta);
        }
    }
}

// Add this frame event handler
static void pointer_handle_frame(void* data, struct wl_pointer* pointer) {
    // Frame event marks the end of a pointer event sequence
    // All events received since the previous frame event belong together logically
    // You can use this to know when to update your UI
}

// Add these handlers for newer Wayland versions (5+)
static void pointer_handle_axis_source(void* data, struct wl_pointer* pointer, uint32_t axis_source) {
    // Indicates the source of the axis event (wheel, finger, continuous)
}

static void pointer_handle_axis_stop(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis) {
    // Indicates that a scroll action has stopped
}

static void pointer_handle_axis_discrete(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete) {
    // Provides discrete step information for axis events
}

// Update your pointer listener with ALL required callbacks
static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_handle_enter,
    .leave = pointer_handle_leave,
    .motion = pointer_handle_motion,
    .button = pointer_handle_button,
    .axis = pointer_handle_axis,
    .frame = pointer_handle_frame,
    .axis_source = pointer_handle_axis_source,
    .axis_stop = pointer_handle_axis_stop,
    .axis_discrete = pointer_handle_axis_discrete,
};

// Seat capability handling
static void seat_handle_capabilities(void* data, struct wl_seat* seat, uint32_t capabilities) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    
    // Handle keyboard capability
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        if (!state->keyboard) {
            state->keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
        }
    } else if (state->keyboard) {
        wl_keyboard_destroy(state->keyboard);
        state->keyboard = NULL;
    }
    
    // Handle pointer capability
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        if (!state->pointer) {
            state->pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(state->pointer, &pointer_listener, state);
        }
    } else if (state->pointer) {
        wl_pointer_destroy(state->pointer);
        state->pointer = NULL;
    }
}

// Define a proper seat listener structure with ALL required callbacks
// The seat interface typically has two events: capabilities and name
static void seat_handle_name(void *data, struct wl_seat *wl_seat, const char *name) {
    // This is just informational, you can log it if you want
    PRINT_DEBUG("Wayland seat name: %s", name);
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name = seat_handle_name,
};

// Registry handling
static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)data;
    
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &wm_base_listener, NULL);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->seat = wl_registry_bind(registry, id, &wl_seat_interface, 5);
        wl_seat_add_listener(state->seat, &seat_listener, state);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    }
}

static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    // Handle global object removal if needed
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

b8 platform_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x, i32 y,
    i32 width, i32 height) {
    
    *memory_requirement = sizeof(PLATFORM_STATE);
    if (state == 0) {
        return true;
    }
    
    state_ptr = state;
    memset(state_ptr, 0, sizeof(PLATFORM_STATE));
    
    // Store window dimensions
    state_ptr->width = width;
    state_ptr->height = height;
    
    // Connect to Wayland display
    state_ptr->display = wl_display_connect(NULL);
    if (!state_ptr->display) {
        PRINT_ERROR("Failed to connect to Wayland display");
        return false;
    }
    
    // Get Wayland registry
    state_ptr->registry = wl_display_get_registry(state_ptr->display);
    wl_registry_add_listener(state_ptr->registry, &registry_listener, state_ptr);
    
    // First roundtrip to get registry objects
    wl_display_roundtrip(state_ptr->display);
    
    // Check if we got the required interfaces
    if (!state_ptr->compositor) {
        PRINT_ERROR("No Wayland compositor found");
        return false;
    }
    
    if (!state_ptr->xdg_wm_base) {
        PRINT_ERROR("No XDG shell found");
        return false;
    }
    
    // Create cursor theme if shm is available
    if (state_ptr->shm) {
        state_ptr->cursor_theme = wl_cursor_theme_load(NULL, 24, state_ptr->shm);
        if (state_ptr->cursor_theme) {
            state_ptr->default_cursor = wl_cursor_theme_get_cursor(state_ptr->cursor_theme, "left_ptr");
            if (state_ptr->default_cursor) {
                state_ptr->cursor_surface = wl_compositor_create_surface(state_ptr->compositor);
            }
        }
    }
    
    // Create surfaces
    state_ptr->surface = wl_compositor_create_surface(state_ptr->compositor);
    if (!state_ptr->surface) {
        PRINT_ERROR("Failed to create Wayland surface");
        return false;
    }
    
    // Create XDG surface
    state_ptr->xdg_surface = xdg_wm_base_get_xdg_surface(state_ptr->xdg_wm_base, state_ptr->surface);
    if (!state_ptr->xdg_surface) {
        PRINT_ERROR("Failed to create XDG surface");
        return false;
    }
    
    xdg_surface_add_listener(state_ptr->xdg_surface, &xdg_surface_listener, state_ptr);
    
    // Create XDG toplevel
    state_ptr->xdg_toplevel = xdg_surface_get_toplevel(state_ptr->xdg_surface);
    if (!state_ptr->xdg_toplevel) {
        PRINT_ERROR("Failed to create XDG toplevel");
        return false;
    }
    
    xdg_toplevel_add_listener(state_ptr->xdg_toplevel, &xdg_toplevel_listener, state_ptr);
    xdg_toplevel_set_title(state_ptr->xdg_toplevel, application_name);
    xdg_toplevel_set_app_id(state_ptr->xdg_toplevel, "YwmaaEngine");
    
    // Set window size
    if (width > 0 && height > 0) {
        xdg_toplevel_set_min_size(state_ptr->xdg_toplevel, width, height);
    }
    
    // Commit surface to apply changes
    wl_surface_commit(state_ptr->surface);
    
    // Second roundtrip to get surface created
    wl_display_roundtrip(state_ptr->display);
    
    PRINT_INFO("Wayland platform initialized successfully");
    return true;
}

void platform_system_shutdown(void* platform_state) {
    if (!state_ptr) {
        return;
    }
    
    // Clean up Wayland resources
    if (state_ptr->keyboard) {
        wl_keyboard_destroy(state_ptr->keyboard);
    }
    
    if (state_ptr->pointer) {
        wl_pointer_destroy(state_ptr->pointer);
    }
    
    if (state_ptr->seat) {
        wl_seat_release(state_ptr->seat);
    }
    
    if (state_ptr->cursor_surface) {
        wl_surface_destroy(state_ptr->cursor_surface);
    }
    
    if (state_ptr->cursor_theme) {
        wl_cursor_theme_destroy(state_ptr->cursor_theme);
    }
    
    if (state_ptr->xdg_toplevel) {
        xdg_toplevel_destroy(state_ptr->xdg_toplevel);
    }
    
    if (state_ptr->xdg_surface) {
        xdg_surface_destroy(state_ptr->xdg_surface);
    }
    
    if (state_ptr->surface) {
        wl_surface_destroy(state_ptr->surface);
    }
    
    if (state_ptr->xdg_wm_base) {
        xdg_wm_base_destroy(state_ptr->xdg_wm_base);
    }
    
    if (state_ptr->shm) {
        wl_shm_destroy(state_ptr->shm);
    }
    
    if (state_ptr->compositor) {
        wl_compositor_destroy(state_ptr->compositor);
    }
    
    if (state_ptr->registry) {
        wl_registry_destroy(state_ptr->registry);
    }
    
    if (state_ptr->display) {
        wl_display_disconnect(state_ptr->display);
    }
    
    PRINT_INFO("Wayland platform shutdown complete");
}

b8 platform_pump_messages() {
    if (!state_ptr || !state_ptr->display) {
        return true;
    }
    
    // Process all pending Wayland events
    while (wl_display_prepare_read(state_ptr->display) != 0) {
        wl_display_dispatch_pending(state_ptr->display);
    }
    
    wl_display_flush(state_ptr->display);
    
    if (wl_display_read_events(state_ptr->display) != 0) {
        PRINT_ERROR("Failed to read Wayland events");
        return false;
    }
    
    wl_display_dispatch_pending(state_ptr->display);
    
    // Check if window was closed
    if (state_ptr->closed) {
        return false;
    }
    
    return true;
}

void* platform_allocate(u64 size, b8 aligned) {
    return malloc(size);
}

void platform_free(void* block, b8 aligned) {
    free(block);
}

void* platform_zero_memory(void* block, u64 size) {
    return memset(block, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, u64 size) {
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

f64 platform_get_absolute_time() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void platform_sleep(u64 ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

void platform_get_required_extension_names(const char ***names_darray) {
    darray_push(*names_darray, &"VK_KHR_wayland_surface");
}

// Surface creation for Vulkan
b8 platform_create_vulkan_surface(VULKAN_CONTEXT *context) {
    if (!state_ptr) {
        return false;
    }

    VkWaylandSurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
    create_info.display = state_ptr->display;
    create_info.surface = state_ptr->surface;

    // Use vkCreateWaylandSurfaceKHR from Volk
    VkResult result = vkCreateWaylandSurfaceKHR(
        context->instance,
        &create_info,
        context->allocator,
        &state_ptr->vulkan_surface);
        
    if (result != VK_SUCCESS) {
        PRINT_ERROR("Vulkan Wayland surface creation failed.");
        return false;
    }

    context->surface = state_ptr->vulkan_surface;
    return true;
}

// Surface creation for WebGPU
b8 platform_create_webgpu_surface(WEBGPU_CONTEXT *context) {
    if (!state_ptr) {
        return false;
    }

    WGPUSurfaceDescriptorFromWaylandSurface fromWaylandSurface;
    fromWaylandSurface.chain.next = NULL;
    fromWaylandSurface.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
    fromWaylandSurface.display = state_ptr->display;
    fromWaylandSurface.surface = state_ptr->surface;

    WGPUSurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &fromWaylandSurface.chain;
    surfaceDescriptor.label = NULL;

    state_ptr->webgpu_surface = wgpuInstanceCreateSurface(context->instance, &surfaceDescriptor);

    context->surface = state_ptr->webgpu_surface;
    return true;
}

#endif // defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED) && !defined(YPLATFORM_ANDROID)
