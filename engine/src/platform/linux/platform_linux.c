/* PLATFORM LINUX.c
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.15 -02:05
 * Last edited:
 *   <l3572PMle>
 * Auto updated?
 *   Yes
 *
 * Description:
 *   Linux Platform Layer
**/

#include "../platform.h"
#include "platform_linux.h"

// Linux platform layer - coordinator for Wayland/X11 fallback
#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

typedef struct LINUX_FILE_WATCH {
    u32 id;
    const char *file_path;
    long last_write_time;
} LINUX_FILE_WATCH;

// darray
static LINUX_FILE_WATCH* file_watches;

void platform_update_watches(void);

// Track which platform backend is active

static E_PLATFORM_BACKEND active_backend = PLATFORM_BACKEND_NONE;

E_PLATFORM_BACKEND platform_get_linux_active_display_protocol(void) {
    return active_backend;
}

// Global state pointer that will be used by both backends
static void* state_ptr = NULL;

// Main implementation that tries Wayland first, then X11
b8 platform_system_startup(u64* memory_requirement, void* state, void* config) {
    PLATFORM_SYSTEM_CONFIG* typed_config = (PLATFORM_SYSTEM_CONFIG*)config;
    
    // First call - just get memory requirements
    if (state == 0) {
        // Use the larger of the two requirements
        u64 wayland_req = 0;
        u64 x11_req = 0;
        
        #ifdef WAYLAND_ENABLED
        platform_wayland_system_startup(&wayland_req, 0, 0, 0, 0, 0, 0);
        #endif
        
        platform_x11_system_startup(&x11_req, 0, 0, 0, 0, 0, 0);
        
        *memory_requirement = wayland_req > x11_req ? wayland_req : x11_req;
    return true;
}

    // Store the state pointer globally
    state_ptr = state;
    
    // Actual initialization - try Wayland first if enabled
    #ifdef WAYLAND_ENABLED
    PRINT_INFO("Attempting to start with Wayland...");
    if (platform_wayland_system_startup(memory_requirement, state, typed_config->application_name, typed_config->x, typed_config->y, typed_config->width, typed_config->height)) {
        PRINT_INFO("Successfully initialized Wayland platform.");
        active_backend = PLATFORM_BACKEND_WAYLAND;
        return true;
    }
    PRINT_INFO("Wayland initialization failed, falling back to X11...");
    #endif
    
    // Fall back to X11
    PRINT_INFO("Starting with X11...");
    if (platform_x11_system_startup(memory_requirement, state, typed_config->application_name, typed_config->x, typed_config->y, typed_config->width, typed_config->height)) {
        PRINT_INFO("Successfully initialized X11 platform.");
        active_backend = PLATFORM_BACKEND_X11;
        return true;
    }
    
    PRINT_ERROR("Failed to initialize any platform backend!");
    return false;
}

void platform_system_shutdown(void* platform_state) {
    switch (active_backend) {
        case PLATFORM_BACKEND_WAYLAND:
            #ifdef WAYLAND_ENABLED
            platform_wayland_system_shutdown(platform_state);
            #endif
            break;
        case PLATFORM_BACKEND_X11:
            platform_x11_system_shutdown(platform_state);
            break;
        default:
            break;
    }
    active_backend = PLATFORM_BACKEND_NONE;
    state_ptr = NULL;
}

b8 platform_pump_messages(void) {
    // Update watches.
    platform_update_watches();
    switch (active_backend) {
        case PLATFORM_BACKEND_WAYLAND:
            #ifdef WAYLAND_ENABLED
            return platform_wayland_pump_messages();
            #endif
            break;
        case PLATFORM_BACKEND_X11:
            return platform_x11_pump_messages();
            break;
        default:
            break;
    }
    return false;
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

void platform_console_write(const char* message, u8 color) {
    // ERROR,WARN,INFO,DEBUG,TRACE
    const char* color_strings[] = {"1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

f64 platform_get_absolute_time(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
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

i32 platform_get_processor_count(void) {
    // Load processor info.
    i32 processor_count = get_nprocs_conf();
    i32 processors_available = get_nprocs();
    PRINT_INFO("%i processor cores detected, %i cores available.", processor_count, processors_available);
    return processors_available;
}

void platform_get_handle_info(u64 *out_size, void *memory) {
    switch (active_backend) {
        case PLATFORM_BACKEND_WAYLAND:
            #ifdef WAYLAND_ENABLED
            platform_wayland_get_handle_info(out_size, memory);
            #endif
            break;
            
        case PLATFORM_BACKEND_X11:
        default:
            platform_x11_get_handle_info(out_size, memory);
            break;
    }
}

const char* platform_dynamic_library_extension(void) {
    return ".so";
}

const char *platform_dynamic_library_prefix(void) {
    return "./lib";
}

E_PLATFORM_ERROR_CODE platform_copy_file(const char* source, const char* dest, b8 overwrite_if_exists) {
    E_PLATFORM_ERROR_CODE ret_code = PLATFORM_ERROR_SUCCESS;
    i32 source_fd = -1;
    i32 dest_fd = -1;

    // Obtain a file descriptor for the source file.
    source_fd = open(source, O_RDONLY);
    if (source_fd == -1) {
        if (errno == ENOENT) {
            PRINT_ERROR("Source file does not exist: %s", source);
        }
        return PLATFORM_ERROR_FILE_NOT_FOUND;
    }

    // Stat the file to obtain it's attributes (e.g. size).
    struct stat source_stat;
    i32 result = fstat(source_fd, &source_stat);
    if (result != 0) {
        if (errno == ENOENT) {
            PRINT_ERROR("Source file does not exist: %s", source);
        }
        ret_code = PLATFORM_ERROR_FILE_NOT_FOUND;
        goto close_handles;
    }

    u64 size = (u64)source_stat.st_size;

    // Obtain a file descriptor for the source file.
    dest_fd = open(dest, O_WRONLY | O_CREAT);
    if (dest_fd == -1) {
        if (errno == ENOENT) {
            PRINT_ERROR("Destination file could not be created: %s", dest);
        }

        ret_code = PLATFORM_ERROR_FILE_LOCKED;
        goto close_handles;
    }

    // Copy the data. Iterate to handle large files, since Linux has a limit
    // on the amount that can be copied at once.
    while (size > 0) {
        ssize_t sent = sendfile(dest_fd, source_fd, NULL, (size >= SSIZE_MAX ? SSIZE_MAX : (size_t)size));
        if (sent < 0) {
            if (errno != EINVAL && errno != ENOSYS) {
                ret_code = PLATFORM_ERROR_UNKNOWN;
                goto close_handles;
            } else {
                break;
            }
        } else {
            YASSERT((size_t)sent <= size);
            size -= (size_t)sent;
        }
    }

    // Copy file times. Stat the source file again to make sure it's up to date.
    result = fstat(source_fd, &source_stat);
    if (result != 0) {
        ret_code = PLATFORM_ERROR_FILE_NOT_FOUND;
        goto close_handles;
    } else {
        struct timespec dest_times[2];

        // Access time
        dest_times[0] = source_stat.st_atim;

        // Modification time
        dest_times[1] = source_stat.st_mtim;

        result = futimens(dest_fd, dest_times);
        // If an error is returned, treat as the destination file being locked.
        if (result != 0) {
            ret_code = PLATFORM_ERROR_FILE_LOCKED;
            goto close_handles;
        }
    }

    // Copy permissions.
    result = fchmod(dest_fd, source_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
    // If an error is returned, treat as the destination file being locked.
    if (result != 0) {
        ret_code = PLATFORM_ERROR_FILE_LOCKED;
        goto close_handles;
    }

close_handles:
    if (source_fd != -1) {
        result = close(source_fd);
        if (result != 0) {
            PRINT_ERROR("Error closing source file: %s", source);
        }
    }
    if (dest_fd != -1) {
        result = close(dest_fd);
        if (result != 0) {
            PRINT_ERROR("Error closing destination file: %s", source);
        }
    }

    return ret_code;
}

static b8 register_watch(const char *file_path, u32 *out_watch_id) {
    if (!state_ptr || !file_path || !out_watch_id) {
        if (out_watch_id) {
            *out_watch_id = INVALID_ID;
        }
        return false;
    }
    *out_watch_id = INVALID_ID;

    if (!file_watches) {
        file_watches = darray_create(LINUX_FILE_WATCH);
    }

    struct stat info;
    int result = stat(file_path, &info);
    if(result != 0) {
        if(errno == ENOENT) {
            // File doesn't exist. TODO: report?
        }
        return false;
    }

    u32 count = darray_length(file_watches);
    for (u32 i = 0; i < count; ++i) {
        LINUX_FILE_WATCH *w = &file_watches[i];
        if (w->id == INVALID_ID) {
            // Found a free slot to use.
            w->id = i;
            w->file_path = string_duplicate(file_path);
            w->last_write_time = info.st_mtime;
            *out_watch_id = i;
            return true;
        }
    }

    // If no empty slot is available, create and push a new entry.
    LINUX_FILE_WATCH w = {0};
    w.id = count;
    w.file_path = string_duplicate(file_path);
    w.last_write_time = info.st_mtime;
    *out_watch_id = count;
    darray_push(file_watches, w);

    return true;
}

static b8 unregister_watch(u32 watch_id) {
    if (!state_ptr || !file_watches) {
        return false;
    }

    u32 count = darray_length(file_watches);
    if (count == 0 || watch_id > (count - 1)) {
        return false;
    }

    LINUX_FILE_WATCH *w = &file_watches[watch_id];
    w->id = INVALID_ID;
    yfree((void *)w->file_path);
    w->file_path = 0;
    yzero_memory(&w->last_write_time, sizeof(long));

    return true;
}

b8 platform_watch_file(const char *file_path, u32 *out_watch_id) {
    return register_watch(file_path, out_watch_id);
}

b8 platform_unwatch_file(u32 watch_id) {
    return unregister_watch(watch_id);
}

void platform_update_watches(void) {
    if (!state_ptr || !file_watches) {
        return;
    }

    u32 count = darray_length(file_watches);
    for (u32 i = 0; i < count; ++i) {
        LINUX_FILE_WATCH *f = &file_watches[i];
        if (f->id != INVALID_ID) {

            struct stat info;
            int result = stat(f->file_path, &info);
            if(result != 0) {
                if(errno == ENOENT) {
                    // File doesn't exist. Which means it was deleted. Remove the watch.
                    EVENT_CONTEXT context = {0};
                    context.data.u32[0] = f->id;
                    event_fire(EVENT_CODE_WATCHED_FILE_DELETED, 0, context);
                    PRINT_INFO("File watch id %d has been removed.", f->id);
                    unregister_watch(f->id);
                    continue;
                } else {
                    PRINT_WARNING("Some other error occurred on file watch id %d", f->id);
                }
                // NOTE: some other error has occurred. TODO: Handle?
                continue;
            }

            // Check the file time to see if it has been changed and update/notify if so.
            if (info.st_mtime - f->last_write_time != 0) {
                PRINT_TRACE("File update found.");
                f->last_write_time = info.st_mtime;
                // Notify listeners.
                EVENT_CONTEXT context = {0};
                context.data.u32[0] = f->id;
                event_fire(EVENT_CODE_WATCHED_FILE_WRITTEN, 0, context);
            }
        }
    }
}

// NOTE: Begin threads.

b8 ythread_create(PFN_THREAD_START start_function_ptr, void* params, b8 auto_detach, YTHREAD* out_thread) {
    if (!start_function_ptr) {
        return false;
    }

    // pthread_create uses a function pointer that returns void*, so cold-cast to this type.
    i32 result = pthread_create((pthread_t*)&out_thread->thread_id, 0, (void* (*)(void*))start_function_ptr, params);
    if (result != 0) {
        switch (result) {
            case EAGAIN:
                PRINT_ERROR("Failed to create thread: insufficient resources to create another thread.");
                return false;
            case EINVAL:
                PRINT_ERROR("Failed to create thread: invalid settings were passed in attributes..");
                return false;
            default:
                PRINT_ERROR("Failed to create thread: an unhandled error has occurred. errno=%i", result);
                return false;
        }
    }
    PRINT_DEBUG("Starting process on thread id: %#x", out_thread->thread_id);

    // Only save off the handle if not auto-detaching.
    if (!auto_detach) {
        out_thread->internal_data = platform_allocate(sizeof(u64), false);
        *(u64*)out_thread->internal_data = out_thread->thread_id;
    } else {
        // If immediately detaching, make sure the operation is a success.
        result = pthread_detach(*(pthread_t*)out_thread->internal_data);
        if (result != 0) {
            switch (result) {
                case EINVAL:
                    PRINT_ERROR("Failed to detach newly-created thread: thread is not a joinable thread.");
                    return false;
                case ESRCH:
                    PRINT_ERROR("Failed to detach newly-created thread: no thread with the id %#x could be found.", out_thread->thread_id);
                    return false;
                default:
                    PRINT_ERROR("Failed to detach newly-created thread: an unknown error has occurred. errno=%i", result);
                    return false;
            }
        }
    }

    return true;
}

void ythread_destroy(YTHREAD* thread) {
    ythread_cancel(thread);
}

void ythread_detach(YTHREAD* thread) {
    if (thread->internal_data) {
        i32 result = pthread_detach(*(pthread_t*)thread->internal_data);
        if (result != 0) {
            switch (result) {
                case EINVAL:
                    PRINT_ERROR("Failed to detach thread: thread is not a joinable thread.");
                    break;
                case ESRCH:
                    PRINT_ERROR("Failed to detach thread: no thread with the id %#x could be found.", thread->thread_id);
                    break;
                default:
                    PRINT_ERROR("Failed to detach thread: an unknown error has occurred. errno=%i", result);
                    break;
            }
        }
        platform_free(thread->internal_data, false);
        thread->internal_data = 0;
    }
}

void ythread_cancel(YTHREAD* thread) {
    if (thread->internal_data) {
        i32 result = pthread_cancel(*(pthread_t*)thread->internal_data);
        if (result != 0) {
            switch (result) {
                case ESRCH:
                    PRINT_ERROR("Failed to cancel thread: no thread with the id %#x could be found.", thread->thread_id);
                    break;
                default:
                    PRINT_ERROR("Failed to cancel thread: an unknown error has occurred. errno=%i", result);
                    break;
            }
        }
        platform_free(thread->internal_data, false);
        thread->internal_data = 0;
        thread->thread_id = 0;
    }
}

b8 ythread_is_active(YTHREAD* thread) {
    // TODO: Find a better way to verify this.
    return thread->internal_data != 0;
}

void ythread_sleep(YTHREAD* thread, u64 ms) {
    platform_sleep(ms);
}

u64 get_thread_id(void) {
    return (u64)pthread_self();
}
// NOTE: End threads.


// NOTE: Begin mutexes
b8 ymutex_create(YMUTEX* out_mutex) {
    if (!out_mutex) {
        return false;
    }

    // Initialize
    pthread_mutex_t mutex;
    i32 result = pthread_mutex_init(&mutex, 0);
    if (result != 0) {
        PRINT_ERROR("Mutex creation failure!");
        return false;
    }

    // Save off the mutex handle.
    out_mutex->internal_data = platform_allocate(sizeof(pthread_mutex_t), false);
    *(pthread_mutex_t*)out_mutex->internal_data = mutex;

    return true;
}

void ymutex_destroy(YMUTEX* mutex) {
    if (mutex) {
        i32 result = pthread_mutex_destroy((pthread_mutex_t*)mutex->internal_data);
        switch (result) {
            case 0:
                // PRINT_TRACE("Mutex destroyed.");
                break;
            case EBUSY:
                PRINT_ERROR("Unable to destroy mutex: mutex is locked or referenced.");
                break;
            case EINVAL:
                PRINT_ERROR("Unable to destroy mutex: the value specified by mutex is invalid.");
                break;
            default:
                PRINT_ERROR("An handled error has occurred while destroy a mutex: errno=%i", result);
                break;
        }

        platform_free(mutex->internal_data, false);
        mutex->internal_data = 0;
    }
}

b8 ymutex_lock(YMUTEX* mutex) {
    if (!mutex) {
        return false;
    }
    // Lock
    i32 result = pthread_mutex_lock((pthread_mutex_t*)mutex->internal_data);
    switch (result) {
        case 0:
            // Success, everything else is a failure.
            // PRINT_TRACE("Obtained mutex lock.");
            return true;
        case EOWNERDEAD:
            PRINT_ERROR("Owning thread terminated while mutex still active.");
            return false;
        case EAGAIN:
            PRINT_ERROR("Unable to obtain mutex lock: the maximum number of recursive mutex locks has been reached.");
            return false;
        case EBUSY:
            PRINT_ERROR("Unable to obtain mutex lock: a mutex lock already exists.");
            return false;
        case EDEADLK:
            PRINT_ERROR("Unable to obtain mutex lock: a mutex deadlock was detected.");
            return false;
        default:
            PRINT_ERROR("An handled error has occurred while obtaining a mutex lock: errno=%i", result);
            return false;
    }
}

b8 ymutex_unlock(YMUTEX* mutex) {
    if (!mutex) {
        return false;
    }
    if (mutex->internal_data) {
        i32 result = pthread_mutex_unlock((pthread_mutex_t*)mutex->internal_data);
        switch (result) {
            case 0:
                // PRINT_TRACE("Freed mutex lock.");
                return true;
            case EOWNERDEAD:
                PRINT_ERROR("Unable to unlock mutex: owning thread terminated while mutex still active.");
                return false;
            case EPERM:
                PRINT_ERROR("Unable to unlock mutex: mutex not owned by current thread.");
                return false;
            default:
                PRINT_ERROR("An handled error has occurred while unlocking a mutex lock: errno=%i", result);
                return false;
        }
    }

    return false;
}
// NOTE: End mutexes

#endif // defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

// Linux platform layer - X11 implementation
#if defined(YPLATFORM_LINUX) && !defined(YPLATFORM_ANDROID)

// if X11 is loaded dynamically then define this
#define X11_DYNAMIC_LOAD_IMPLEMENTATION
#include "../../../thirdparty/x11_loader/X11_loader.h"


typedef struct X11_HANDLE_INFO {
    xcb_connection_t* connection;
    xcb_window_t window;
} X11_HANDLE_INFO;
typedef struct PLATFORM_STATE {
    Display* display;
    X11_HANDLE_INFO handle;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} PLATFORM_STATE;

// Key translation
E_KEYS translate_keycode(u32 x_keycode);

b8 platform_x11_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {
    *memory_requirement = sizeof(PLATFORM_STATE);
    if (state == 0) {
        return true;
    }

    state_ptr = state;
    // Load X11
    X11_LOADER_RESULT result = x11_initialize();
    if (result == X11_LOADER_LIBX11_ERROR){
        PRINT_ERROR("libX11 not found");
    } else if (result == X11_LOADER_LIBXCB_ERROR)
    {
        PRINT_ERROR("libxcb not found");
    } else if (result == X11_LOADER_LIBX11XCB_ERROR)
    {
        PRINT_ERROR("libX11-xcb not found");
    }
    else {
        PRINT_INFO("Loaded X11");
    }
    
    if (result != X11_LOADER_SUCCESS) {
        PRINT_ERROR("Failed to load X11");
        return false;
    }

    // Connect to X
    PLATFORM_STATE* platform_state = (PLATFORM_STATE*)state;
    platform_state->display = loader_XOpenDisplay(NULL);
    if (!platform_state->display) {
        PRINT_ERROR("Failed to connect to X server via XOpenDisplay");
        return false;
    }

    // Retrieve the connection from the display.
    platform_state->handle.connection = loader_XGetXCBConnection(platform_state->display);
    if (!platform_state->handle.connection) {
        PRINT_ERROR("Failed to get XCB connection from display");
        return false;
    }

    // TODO: Fix this check crashes
    if (loader_xcb_connection_has_error(platform_state->handle.connection)) {
        PRINT_ERROR("Failed to connect to X server via XCB.");
        return false;
    }
    
    // Get data from the X server
    const struct xcb_setup_t* setup = loader_xcb_get_setup(platform_state->handle.connection);

    // Loop through screens using iterator
    xcb_screen_iterator_t it = loader_xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for (i32 s = screen_p; s > 0; s--) {
        loader_xcb_screen_next(&it);
    }

    // After screens have been looped through, assign it.
    platform_state->screen = it.data;

    // Allocate a XID for the window to be created.
    platform_state->handle.window = loader_xcb_generate_id(platform_state->handle.connection);

    // Register event types.
    // XCB_CW_BACK_PIXEL = filling then window bg with a single color
    // XCB_CW_EVENT_MASK is required.
    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // Listen for keyboard and mouse buttons
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                       XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Values to be sent over XCB (bg color, events)
    u32 value_list[] = {platform_state->screen->black_pixel, event_values};

    // Create the window
    loader_xcb_create_window(
        platform_state->handle.connection,
        XCB_COPY_FROM_PARENT,  // depth
        platform_state->handle.window,
        platform_state->screen->root,            // parent
        x,                              //x
        y,                              //y
        width,                          //width
        height,                         //height
        0,                              // No border
        XCB_WINDOW_CLASS_INPUT_OUTPUT,  //class
        platform_state->screen->root_visual,
        event_mask,
        value_list);

    // Change the title
    loader_xcb_change_property(
        platform_state->handle.connection,
        XCB_PROP_MODE_REPLACE,
        platform_state->handle.window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,  // data should be viewed 8 bits at a time
        strlen(application_name),
        application_name);

    // Tell the server to notify when the window manager
    // attempts to destroy the window.
    xcb_intern_atom_cookie_t wm_delete_cookie = loader_xcb_intern_atom(
        platform_state->handle.connection,
        0,
        strlen("WM_DELETE_WINDOW"),
        "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = loader_xcb_intern_atom(
        platform_state->handle.connection,
        0,
        strlen("WM_PROTOCOLS"),
        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = loader_xcb_intern_atom_reply(
        platform_state->handle.connection,
        wm_delete_cookie,
        NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = loader_xcb_intern_atom_reply(
        platform_state->handle.connection,
        wm_protocols_cookie,
        NULL);
    platform_state->wm_delete_win = wm_delete_reply->atom;
    platform_state->wm_protocols = wm_protocols_reply->atom;

    loader_xcb_change_property(
        platform_state->handle.connection,
        XCB_PROP_MODE_REPLACE,
        platform_state->handle.window,
        wm_protocols_reply->atom,
        4,
        32,
        1,
        &wm_delete_reply->atom);

    // Map the window to the screen
    loader_xcb_map_window(platform_state->handle.connection, platform_state->handle.window);

    // Fire initial resize event
    EVENT_CONTEXT context;
    context.data.u16[0] = width;
    context.data.u16[1] = height;
    event_fire(EVENT_CODE_RESIZED, 0, context);

    // Flush the stream
    i32 stream_result = loader_xcb_flush(platform_state->handle.connection);
    if (stream_result <= 0) {
        PRINT_ERROR("An error occurred when flusing the stream: %d", stream_result);
        return false;
    }

    return true;
}

void platform_x11_system_shutdown(void* platform_state) {
    if (platform_state) {
        PLATFORM_STATE* state = (PLATFORM_STATE*)platform_state;
        loader_xcb_destroy_window(state->handle.connection, state->handle.window);
    }
}

b8 platform_x11_pump_messages(void) {
    if (state_ptr) {
        PLATFORM_STATE* state = (PLATFORM_STATE*)state_ptr;
        xcb_generic_event_t* event;
        xcb_client_message_event_t* cm;

        b8 quit_flagged = false;

        // Poll for events until null is returned.
        while ((event = loader_xcb_poll_for_event(state->handle.connection))) {
            // Input events
            switch (event->response_type & ~0x80) {
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE: {
                    // Key press event - xcb_key_press_event_t and xcb_key_release_event_t are the same
                    xcb_key_press_event_t* kb_event = (xcb_key_press_event_t*)event;
                    b8 pressed = event->response_type == XCB_KEY_PRESS;
                    xcb_keycode_t code = kb_event->detail;
                    KeySym key_sym = loader_XkbKeycodeToKeysym(
                        state->display,
                        (KeyCode)code,  //event.xkey.keycode,
                        0,
                        0/* code & ShiftMask ? 1 : 0 */);

                    E_KEYS key = translate_keycode(key_sym);

                    // Pass to the input subsystem for processing.
                    input_process_key(key, pressed);
                } break;
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: {
                    xcb_button_press_event_t* mouse_event = (xcb_button_press_event_t*)event;
                    b8 pressed = event->response_type == XCB_BUTTON_PRESS;
                    E_BUTTONS mouse_button = BUTTON_MAX_BUTTONS;
                    switch (mouse_event->detail) {
                        case XCB_BUTTON_INDEX_1:
                            mouse_button = BUTTON_LEFT;
                            break;
                        case XCB_BUTTON_INDEX_2:
                            mouse_button = BUTTON_MIDDLE;
                            break;
                        case XCB_BUTTON_INDEX_3:
                            mouse_button = BUTTON_RIGHT;
                            break;
                    }

                    // Pass over to the input subsystem.
                    if (mouse_button != BUTTON_MAX_BUTTONS) {
                        input_process_button(mouse_button, pressed);
                    }
                } break;
                case XCB_MOTION_NOTIFY: {
                    // Mouse move
                    xcb_motion_notify_event_t* move_event = (xcb_motion_notify_event_t*)event;

                    // Pass over to the input subsystem.
                    input_process_mouse_move(move_event->event_x, move_event->event_y);
                } break;
                case XCB_CONFIGURE_NOTIFY: {
                    // Resizing - note that this is also triggered by moving the window, but should be
                    // passed anyway since a change in the x/y could mean an upper-left resize.
                    // The application layer can decide what to do with this.
                    xcb_configure_notify_event_t* configure_event = (xcb_configure_notify_event_t*)event;

                    // Fire the event. The application layer should pick this up, but not handle it
                    // as it shouldn be visible to other parts of the application.
                    EVENT_CONTEXT context;
                    context.data.u16[0] = configure_event->width;
                    context.data.u16[1] = configure_event->height;
                    event_fire(EVENT_CODE_RESIZED, 0, context);

                } break;

                case XCB_CLIENT_MESSAGE: {
                    cm = (xcb_client_message_event_t*)event;

                    // Window close
                    if (cm->data.data32[0] == state->wm_delete_win) {
                        quit_flagged = true;
                    }
                } break;
                default:
                    // Something else
                    break;
            }
            
            free(event);
        }

        return !quit_flagged;
    }
    
    return false;
}

void platform_x11_get_handle_info(u64 *out_size, void *memory) {

    *out_size = sizeof(X11_HANDLE_INFO);
    
    if (!memory) {
        return;
    }
    PLATFORM_STATE* platform_state = (PLATFORM_STATE*)state_ptr;

    ycopy_memory(memory, &platform_state->handle, *out_size);
}

// Key translation
E_KEYS translate_keycode(u32 x_keycode) {
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
            return KEY_PAGEUP;
        case XK_Next:
            return KEY_PAGEDOWN;
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
            return KEY_LSUPER;
        case XK_Super_R:
            return KEY_RSUPER;
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
        case XK_Alt_L:
            return KEY_LALT;
        case XK_Alt_R:
            return KEY_RALT;

        case XK_semicolon:
            return KEY_SEMICOLON;
        case XK_plus:
            return KEY_EQUAL;
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

        case XK_0:
            return KEY_0;
        case XK_1:
            return KEY_1;
        case XK_2:
            return KEY_2;
        case XK_3:
            return KEY_3;
        case XK_4:
            return KEY_4;
        case XK_5:
            return KEY_5;
        case XK_6:
            return KEY_6;
        case XK_7:
            return KEY_7;
        case XK_8:
            return KEY_8;
        case XK_9:
            return KEY_9;

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

#endif // defined(YPLATFORM_LINUX)

// AI helped me creating this wayland backend and I tweaked stuff and removed useless stuff -for now-
// and still I should probably later do some rewriting and reorganizing of this implementation
// Linux platform layer - Wayland implementation
#if defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED) && !defined(YPLATFORM_ANDROID)

#define WAYLAND_DYNAMIC_LOAD_IMPLEMENTATION
#include "wayland/wayland_loader/wayland_loader.h"
#include "wayland/xdg-shell.h"

typedef struct WAYLAND_HANDLE_INFO {
    struct wl_display* display;
    struct wl_surface* surface;
} WAYLAND_HANDLE_INFO;
typedef struct WAYLAND_PLATFORM_STATE {
    WAYLAND_HANDLE_INFO handle;
    
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    
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
} WAYLAND_PLATFORM_STATE;

// Wayland protocol handlers
static void xdg_surface_handle_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
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
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
    
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
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
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
// I mapped these myself by printing each key's code, not the best but just to get this up and running quickly.
// Probably needs another method or perhaps just use XKB but I don't want to have such dependecy yet.
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
        case 104: return KEY_PAGEUP;
        case 109: return KEY_PAGEDOWN;
        case 107: return KEY_END;
        case 102: return KEY_HOME;
        case 105: return KEY_LEFT;
        case 103: return KEY_UP;
        case 106: return KEY_RIGHT;
        case 108: return KEY_DOWN;
        case 634: return KEY_PRINT;
        case 110: return KEY_INSERT;
        case 111: return KEY_DELETE;
        
        case 79: return KEY_NUMPAD1;
        case 80: return KEY_NUMPAD2;
        case 81: return KEY_NUMPAD3;
        case 75: return KEY_NUMPAD4;
        case 76: return KEY_NUMPAD5;
        case 77: return KEY_NUMPAD6;
        case 71: return KEY_NUMPAD7;
        case 72: return KEY_NUMPAD8;
        case 73: return KEY_NUMPAD9;
        case 82: return KEY_NUMPAD0;

        case 2: return KEY_1;
        case 3: return KEY_2;
        case 4: return KEY_3;
        case 5: return KEY_4;
        case 6: return KEY_5;
        case 7: return KEY_6;
        case 8: return KEY_7;
        case 9: return KEY_8;
        case 10: return KEY_9;
        case 11: return KEY_0;
        
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
        case 125: return KEY_LSUPER;
        //case 125: return KEY_RWIN;
        
        case 39: return KEY_SEMICOLON;
        case 13: return KEY_EQUAL;
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
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
    state->has_focus = true;
}

static void keyboard_handle_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface) {
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
    state->has_focus = false;
}

static void keyboard_handle_key(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    // Convert keycode to our key format and forward to input system
    //PRINT_INFO("%i\n", key);
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
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;

    // PRINT_DEBUG("Mouse entered");
    
    // Set cursor
    if (state->cursor_theme && state->default_cursor) {
        struct wl_cursor_image* image = state->default_cursor->images[0];
        struct wl_buffer* buffer = WAYLAND_wl_cursor_image_get_buffer(image);
        
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
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
    
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
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)data;
    
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

b8 platform_wayland_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x, i32 y,
    i32 width, i32 height) {

        
    *memory_requirement = sizeof(WAYLAND_PLATFORM_STATE);
    if (state == 0) {
        return true;
    }
        
    if (WAYLAND_LoadSymbols() != 1) {
        PRINT_ERROR("Failed to load Wayland symbols");
        return false;
    }
    state_ptr = state;
    WAYLAND_PLATFORM_STATE* platform_state = (WAYLAND_PLATFORM_STATE*)state;
    memset(platform_state, 0, sizeof(WAYLAND_PLATFORM_STATE));
    
    // Store window dimensions
    platform_state->width = width;
    platform_state->height = height;
    
    // Connect to Wayland display
    platform_state->handle.display = WAYLAND_wl_display_connect(NULL);
    if (!platform_state->handle.display) {
        PRINT_ERROR("Failed to connect to Wayland display");
        return false;
    }
    
    // Get Wayland registry
    platform_state->registry = wl_display_get_registry(platform_state->handle.display);
    wl_registry_add_listener(platform_state->registry, &registry_listener, platform_state);
    
    // First roundtrip to get registry objects
    WAYLAND_wl_display_roundtrip(platform_state->handle.display);
    
    // Check if we got the required interfaces
    if (!platform_state->compositor) {
        PRINT_ERROR("No Wayland compositor found");
        return false;
    }
    
    if (!platform_state->xdg_wm_base) {
        PRINT_ERROR("No XDG shell found");
        return false;
    }
    
    // Create cursor theme if shm is available
    if (platform_state->shm) {
        platform_state->cursor_theme = WAYLAND_wl_cursor_theme_load(NULL, 24, platform_state->shm);
        if (platform_state->cursor_theme) {
            platform_state->default_cursor = WAYLAND_wl_cursor_theme_get_cursor(platform_state->cursor_theme, "left_ptr");
            if (platform_state->default_cursor) {
                platform_state->cursor_surface = wl_compositor_create_surface(platform_state->compositor);
            }
        }
    }
    
    // Create surfaces
    platform_state->handle.surface = wl_compositor_create_surface(platform_state->compositor);
    if (!platform_state->handle.surface) {
        PRINT_ERROR("Failed to create Wayland surface");
        return false;
    }
    
    // Create XDG surface
    platform_state->xdg_surface = xdg_wm_base_get_xdg_surface(platform_state->xdg_wm_base, platform_state->handle.surface);
    if (!platform_state->xdg_surface) {
        PRINT_ERROR("Failed to create XDG surface");
        return false;
    }
    
    xdg_surface_add_listener(platform_state->xdg_surface, &xdg_surface_listener, platform_state);
    
    // Create XDG toplevel
    platform_state->xdg_toplevel = xdg_surface_get_toplevel(platform_state->xdg_surface);
    if (!platform_state->xdg_toplevel) {
        PRINT_ERROR("Failed to create XDG toplevel");
        return false;
    }
    
    xdg_toplevel_add_listener(platform_state->xdg_toplevel, &xdg_toplevel_listener, platform_state);
    xdg_toplevel_set_title(platform_state->xdg_toplevel, application_name);
    xdg_toplevel_set_app_id(platform_state->xdg_toplevel, "YwmaaEngine");
    
    // Set window size
    if (width > 0 && height > 0) {
        xdg_toplevel_set_min_size(platform_state->xdg_toplevel, width, height);
    }
    
    // Commit surface to apply changes
    wl_surface_commit(platform_state->handle.surface);
    
    // Second roundtrip to get surface created
    WAYLAND_wl_display_roundtrip(platform_state->handle.display);

    // Fire initial resize event
    EVENT_CONTEXT context;
    context.data.u16[0] = width;
    context.data.u16[1] = height;
    event_fire(EVENT_CODE_RESIZED, 0, context);
    
    PRINT_INFO("Wayland platform initialized successfully");
    return true;
}

void platform_wayland_system_shutdown(void* platform_state) {
    if (!platform_state) {
        return;
    }
    
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)platform_state;
    
    // Clean up Wayland resources
    if (state->keyboard) {
        wl_keyboard_destroy(state->keyboard);
    }
    
    if (state->pointer) {
        wl_pointer_destroy(state->pointer);
    }
    
    if (state->seat) {
        wl_seat_release(state->seat);
    }
    
    if (state->cursor_surface) {
        wl_surface_destroy(state->cursor_surface);
    }
    
    if (state->cursor_theme) {
        WAYLAND_wl_cursor_theme_destroy(state->cursor_theme);
    }
    
    if (state->xdg_toplevel) {
        xdg_toplevel_destroy(state->xdg_toplevel);
    }
    
    if (state->xdg_surface) {
        xdg_surface_destroy(state->xdg_surface);
    }
    
    if (state->handle.surface) {
        wl_surface_destroy(state->handle.surface);
    }
    
    if (state->xdg_wm_base) {
        xdg_wm_base_destroy(state->xdg_wm_base);
    }
    
    if (state->shm) {
        wl_shm_destroy(state->shm);
    }
    
    if (state->compositor) {
        wl_compositor_destroy(state->compositor);
    }
    
    if (state->registry) {
        wl_registry_destroy(state->registry);
    }
    
    if (state->handle.display) {
        WAYLAND_wl_display_disconnect(state->handle.display);
    }
    
    PRINT_INFO("Wayland platform shutdown complete");
}

b8 platform_wayland_pump_messages(void) {
    if (!state_ptr) {
        return false;
    }
    
    WAYLAND_PLATFORM_STATE* state = (WAYLAND_PLATFORM_STATE*)state_ptr;
    
    if (!state->handle.display) {
        return false;
    }
    
    // Process all pending Wayland events
    while (WAYLAND_wl_display_prepare_read(state->handle.display) != 0) {
        WAYLAND_wl_display_dispatch_pending(state->handle.display);
    }
    
    WAYLAND_wl_display_flush(state->handle.display);
    
    if (WAYLAND_wl_display_read_events(state->handle.display) != 0) {
        PRINT_ERROR("Failed to read Wayland events");
        return false;
    }
    
    WAYLAND_wl_display_dispatch_pending(state->handle.display);
    
    // Check if window was closed
    if (state->closed) {
        return false;
    }
    
    return true;
}

void platform_wayland_get_handle_info(u64 *out_size, void *memory) {

    *out_size = sizeof(WAYLAND_HANDLE_INFO);
    
    if (!memory) {
        return;
    }
    WAYLAND_PLATFORM_STATE* platform_state = (WAYLAND_PLATFORM_STATE*)state_ptr;

    ycopy_memory(memory, &platform_state->handle, *out_size);
}

#endif // defined(YPLATFORM_LINUX) && defined(WAYLAND_ENABLED) && !defined(YPLATFORM_ANDROID)
