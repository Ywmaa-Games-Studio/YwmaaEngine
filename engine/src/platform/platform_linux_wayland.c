#include "platform.h"

// Linux platform layer.
#if defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED)

#include "core/logger.h"
#include "core/event.h"
#include "input/input.h"

#include "variants/darray.h"

#include <X11/keysym.h>
#include <wayland-client.h>
#include "xdg-shell.h"
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#else
#include <unistd.h>  // usleep
#endif


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// For surface creation
#define VK_USE_PLATFORM_XCB_KHR
#define VOLK_IMPLEMENTATION
#include "../thirdparty/volk/volk.h"
#include "renderer/vulkan/vulkan_types.inl"
typedef struct INTERNAL_STATE {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct xdg_wm_base* shell;
    struct xdg_surface* shellSurface;
    struct xdg_toplevel* toplevel;
    struct wl_seat* seat;
    struct wl_keyboard* keyboard;
    struct wl_callback* callback;
    VkSurfaceKHR surface;
} INTERNAL_STATE;

INTERNAL_STATE* state;
static int quit = 0;
static int readyToResize = 0;
static int resize = 0;
static int newWidth = 0;
static int newHeight = 0;
static uint32_t width = 1600;
static uint32_t height = 900;
static uint32_t currentFrame = 0;


// Key translation
KEYS translate_keycode(u32 x_keycode);

/* 


void frame_new(void* data, struct wl_callback* cb, uint32_t a);
struct wl_callback_listener cb_list = {
	.done = frame_new
};

void frame_new(void* data, struct wl_callback* cb, uint32_t a) {

	wl_callback_destroy(state->callback);
	state->callback = wl_surface_frame(state->surface);
	wl_callback_add_listener(state->callback, &cb_list, 0);
	
	c++;
	draw();
}

void kb_map(void* data, struct wl_keyboard* kb, uint32_t frmt, int32_t fd, uint32_t sz) {
	
}

void kb_enter(void* data, struct wl_keyboard* kb, uint32_t ser, struct wl_surface* srfc, struct wl_array* keys) {
	
}

void kb_leave(void* data, struct wl_keyboard* kb, uint32_t ser, struct wl_surface* srfc) {
	
}

void kb_key(void* data, struct wl_keyboard* kb, uint32_t ser, uint32_t t, uint32_t key, uint32_t stat) {
	if (key == 1) {
		cls = 1;
	}
	else if (key == 30) {
		PRINT_INFO("a\n");
	}
	else if (key == 32) {
		PRINT_INFO("d\n");
	}
}

void kb_mod(void* data, struct wl_keyboard* kb, uint32_t ser, uint32_t dep, uint32_t lat, uint32_t lock, uint32_t grp) {
	
}

void kb_rep(void* data, struct wl_keyboard* kb, int32_t rate, int32_t del) {
	
}

struct wl_keyboard_listener kb_list = {
	.keymap = kb_map,
	.enter = kb_enter,
	.leave = kb_leave,
	.key = kb_key,
	.modifiers = kb_mod,
	.repeat_info = kb_rep
};

void seat_cap(void* data, struct wl_seat* seat, uint32_t cap) {
	if (cap & WL_SEAT_CAPABILITY_KEYBOARD && !state->keyboard) {
		state->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(state->keyboard, &kb_list, 0);
	}
}

void seat_name(void* data, struct wl_seat* seat, const char * name) {
		
}

struct wl_seat_listener seat_list = {
	seat_cap,
	seat_name
};

 */
static void handleRegistry(
    void* data,
	struct wl_registry* registry,
	uint32_t name,
    const char* interface,
    uint32_t version
);

static const struct wl_registry_listener registryListener = {
    .global = handleRegistry
};




static void handleShellPing(void* data, struct xdg_wm_base* shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener shellListener = {
    .ping = handleShellPing
};

static void handleShellSurfaceConfigure(void* data, struct xdg_surface* shellSurface, uint32_t serial)
{
    xdg_surface_ack_configure(shellSurface, serial);

    if (resize)
    {
        readyToResize = 1;
    }
}

static const struct xdg_surface_listener shellSurfaceListener = {
    .configure = handleShellSurfaceConfigure
};

static void handleToplevelConfigure(
    void* data,
    struct xdg_toplevel* toplevel,
    int32_t width,
    int32_t height,
    struct wl_array* states
)
{
    if (width != 0 && height != 0)
    {
        resize = 1;
        newWidth = width;
        newHeight = height;
    }
}

static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel)
{
    quit = 1;
}

static const struct xdg_toplevel_listener toplevelListener = {
    .configure = handleToplevelConfigure,
    .close = handleToplevelClose
};


static void handleRegistry(
    void* data,
	struct wl_registry* registry,
	uint32_t name,
    const char* interface,
    uint32_t version
)
{
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        state->shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->shell, &shellListener, NULL);
    }
}


b8 platform_startup(
    PLATFORM_STATE* platform_state,
    const char* application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {

    width = width;
    height = height;
    // Create the internal state.
    platform_state->internal_state = malloc(sizeof(INTERNAL_STATE));
    state = (INTERNAL_STATE*)platform_state->internal_state;

    // Connect to wayland
    state->display = wl_display_connect(0);

    if (!state->display) {
        PRINT_ERROR("Failed to connect to wayland server.");
        return FALSE;
    }

    // Retrieve the connection from the display.
    state->registry = wl_display_get_registry(state->display);
    
    wl_registry_add_listener(state->registry, &registryListener, 0);
    wl_display_roundtrip(state->display);

	state->surface = wl_compositor_create_surface(state->compositor);

    state->shellSurface = xdg_wm_base_get_xdg_surface(state->shell, state->surface);
    xdg_surface_add_listener(state->shellSurface, &shellSurfaceListener, NULL);

    state->toplevel = &xdg_surface_get_toplevel(state->shellSurface);
    xdg_toplevel_add_listener(state->toplevel, &toplevelListener, NULL);

    xdg_toplevel_set_title(state->toplevel, "Ywmaa Engine");
    xdg_toplevel_set_app_id(state->toplevel, "Ywmaa Engine");

    wl_surface_commit(state->surface);
    wl_display_roundtrip(state->display);
    wl_surface_commit(state->surface);



/* 	state->callback = wl_surface_frame(state->surface);
	wl_callback_add_listener(state->callback, &cb_list, 0); */


    return TRUE;
}

void platform_shutdown(PLATFORM_STATE* platform_state) {
    // Simply cold-cast to the known type.

/* 	if (state->keyboard) {
		wl_keyboard_destroy(state->keyboard);
	}
	wl_seat_release(state->seat); */

    xdg_toplevel_destroy(state->toplevel);
    xdg_surface_destroy(state->shellSurface);
    wl_surface_destroy(state->surface);
    xdg_wm_base_destroy(state->shell);
    wl_compositor_destroy(state->compositor);
    wl_registry_destroy(state->registry);
    wl_display_disconnect(state->display);

}

b8 platform_pump_messages(PLATFORM_STATE* platform_state) {
    // Simply cold-cast to the known type.
    INTERNAL_STATE* state = (INTERNAL_STATE*)platform_state->internal_state;

    b8 quit_flagged = FALSE;

    // Poll for events until null is returned.
    while (wl_display_dispatch(state->display)) {
        if (quit) {
            break;
        }
    }
    return !quit_flagged;
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

void platform_console_write(const char* message, u8 colour) {
    // ERROR,WARN,INFO,DEBUG
    const char* colour_strings[] = {"1;31", "1;33", "1;32", "1;34"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}
void platform_console_write_error(const char* message, u8 colour) {
    // ERROR,WARN,INFO,DEBUG
    const char* colour_strings[] = {"1;31", "1;33", "1;32", "1;34"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
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
    darray_push(*names_darray, &"VK_KHR_xcb_surface");  // VK_KHR_xlib_surface?
}

// Surface creation for Vulkan
b8 platform_create_vulkan_surface(PLATFORM_STATE *platform_state, VULKAN_CONTEXT *context) {
    // Simply cold-cast to the known type.
    INTERNAL_STATE *state = (INTERNAL_STATE *)platform_state->internal_state;

    VkXcbSurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    create_info.connection = state->connection;
    create_info.window = state->window;

    VkResult result = vkCreateXcbSurfaceKHR(
        context->instance,
        &create_info,
        context->allocator,
        &state->surface);
    if (result != VK_SUCCESS) {
        PRINT_ERROR("Vulkan surface creation failed.");
        return FALSE;
    }

    context->surface = state->surface;
    return TRUE;
}

// Key translation
KEYS translate_keycode(u32 x_keycode) {
    switch (x_keycode) {
        case XK_BackSpace:
            return KEY_BACKSPACE;
        case XK_Return:
            return KEY_ENTER;
        case XK_Tab:
            return KEY_TAB;
            //case XK_Shift: return KEY_SHIFT;
            //case XK_Control: return KEY_CONTROL;

        case XK_Pause:
            return KEY_PAUSE;
        case XK_Caps_Lock:
            return KEY_CAPITAL;

        case XK_Escape:
            return KEY_ESCAPE;

            // Not supported
            // case : return KEY_CONVERT;
            // case : return KEY_NONCONVERT;
            // case : return KEY_ACCEPT;

        case XK_Mode_switch:
            return KEY_MODECHANGE;

        case XK_space:
            return KEY_SPACE;
        case XK_Prior:
            return KEY_PRIOR;
        case XK_Next:
            return KEY_NEXT;
        case XK_End:
            return KEY_END;
        case XK_Home:
            return KEY_HOME;
        case XK_Left:
            return KEY_LEFT;
        case XK_Up:
            return KEY_UP;
        case XK_Right:
            return KEY_RIGHT;
        case XK_Down:
            return KEY_DOWN;
        case XK_Select:
            return KEY_SELECT;
        case XK_Print:
            return KEY_PRINT;
        case XK_Execute:
            return KEY_EXECUTE;
        // case XK_snapshot: return KEY_SNAPSHOT; // not supported
        case XK_Insert:
            return KEY_INSERT;
        case XK_Delete:
            return KEY_DELETE;
        case XK_Help:
            return KEY_HELP;

        case XK_Super_L:
            return KEY_LWIN;
        case XK_Super_R:
            return KEY_RWIN;
            // case XK_apps: return KEY_APPS; // not supported

            // case XK_sleep: return KEY_SLEEP; //not supported

        case XK_KP_0:
            return KEY_NUMPAD0;
        case XK_KP_1:
            return KEY_NUMPAD1;
        case XK_KP_2:
            return KEY_NUMPAD2;
        case XK_KP_3:
            return KEY_NUMPAD3;
        case XK_KP_4:
            return KEY_NUMPAD4;
        case XK_KP_5:
            return KEY_NUMPAD5;
        case XK_KP_6:
            return KEY_NUMPAD6;
        case XK_KP_7:
            return KEY_NUMPAD7;
        case XK_KP_8:
            return KEY_NUMPAD8;
        case XK_KP_9:
            return KEY_NUMPAD9;
        case XK_multiply:
            return KEY_MULTIPLY;
        case XK_KP_Add:
            return KEY_ADD;
        case XK_KP_Separator:
            return KEY_SEPARATOR;
        case XK_KP_Subtract:
            return KEY_SUBTRACT;
        case XK_KP_Decimal:
            return KEY_DECIMAL;
        case XK_KP_Divide:
            return KEY_DIVIDE;
        case XK_F1:
            return KEY_F1;
        case XK_F2:
            return KEY_F2;
        case XK_F3:
            return KEY_F3;
        case XK_F4:
            return KEY_F4;
        case XK_F5:
            return KEY_F5;
        case XK_F6:
            return KEY_F6;
        case XK_F7:
            return KEY_F7;
        case XK_F8:
            return KEY_F8;
        case XK_F9:
            return KEY_F9;
        case XK_F10:
            return KEY_F10;
        case XK_F11:
            return KEY_F11;
        case XK_F12:
            return KEY_F12;
        case XK_F13:
            return KEY_F13;
        case XK_F14:
            return KEY_F14;
        case XK_F15:
            return KEY_F15;
        case XK_F16:
            return KEY_F16;
        case XK_F17:
            return KEY_F17;
        case XK_F18:
            return KEY_F18;
        case XK_F19:
            return KEY_F19;
        case XK_F20:
            return KEY_F20;
        case XK_F21:
            return KEY_F21;
        case XK_F22:
            return KEY_F22;
        case XK_F23:
            return KEY_F23;
        case XK_F24:
            return KEY_F24;

        case XK_Num_Lock:
            return KEY_NUMLOCK;
        case XK_Scroll_Lock:
            return KEY_SCROLL;

        case XK_KP_Equal:
            return KEY_NUMPAD_EQUAL;

        case XK_Shift_L:
            return KEY_LSHIFT;
        case XK_Shift_R:
            return KEY_RSHIFT;
        case XK_Control_L:
            return KEY_LCONTROL;
        case XK_Control_R:
            return KEY_RCONTROL;
        // case XK_Menu: return KEY_LMENU;
        case XK_Menu:
            return KEY_RMENU;

        case XK_semicolon:
            return KEY_SEMICOLON;
        case XK_plus:
            return KEY_PLUS;
        case XK_comma:
            return KEY_COMMA;
        case XK_minus:
            return KEY_MINUS;
        case XK_period:
            return KEY_PERIOD;
        case XK_slash:
            return KEY_SLASH;
        case XK_grave:
            return KEY_GRAVE;

        case XK_a:
        case XK_A:
            return KEY_A;
        case XK_b:
        case XK_B:
            return KEY_B;
        case XK_c:
        case XK_C:
            return KEY_C;
        case XK_d:
        case XK_D:
            return KEY_D;
        case XK_e:
        case XK_E:
            return KEY_E;
        case XK_f:
        case XK_F:
            return KEY_F;
        case XK_g:
        case XK_G:
            return KEY_G;
        case XK_h:
        case XK_H:
            return KEY_H;
        case XK_i:
        case XK_I:
            return KEY_I;
        case XK_j:
        case XK_J:
            return KEY_J;
        case XK_k:
        case XK_K:
            return KEY_K;
        case XK_l:
        case XK_L:
            return KEY_L;
        case XK_m:
        case XK_M:
            return KEY_M;
        case XK_n:
        case XK_N:
            return KEY_N;
        case XK_o:
        case XK_O:
            return KEY_O;
        case XK_p:
        case XK_P:
            return KEY_P;
        case XK_q:
        case XK_Q:
            return KEY_Q;
        case XK_r:
        case XK_R:
            return KEY_R;
        case XK_s:
        case XK_S:
            return KEY_S;
        case XK_t:
        case XK_T:
            return KEY_T;
        case XK_u:
        case XK_U:
            return KEY_U;
        case XK_v:
        case XK_V:
            return KEY_V;
        case XK_w:
        case XK_W:
            return KEY_W;
        case XK_x:
        case XK_X:
            return KEY_X;
        case XK_y:
        case XK_Y:
            return KEY_Y;
        case XK_z:
        case XK_Z:
            return KEY_Z;

        default:
            return 0;
    }
}

#endif // defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED)