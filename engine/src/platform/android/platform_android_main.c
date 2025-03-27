
// Android platform layer.
#if defined(PLATFORM_ANDROID)
#include "../platform.h"
#include "core/logger.h"
#include "entry.h"

#include "android_native_app_glue.h"
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/looper.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "YwmaaEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "YwmaaEngine", __VA_ARGS__))

typedef struct PLATFORM_STATE {
    android_app* app;
    ANativeWindow* window;
    AInputQueue* input_queue;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    bool app_initialized;
    bool has_focus;
} PLATFORM_STATE;

static PLATFORM_STATE* state_ptr;
static bool is_running = false;

// Forward declarations
static bool initialize_display(PLATFORM_STATE* state);
static void terminate_display(PLATFORM_STATE* state);

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
    LOGI("Android platform startup: %s", application_name);
    
    return true;
}

void platform_system_shutdown(void* platform_state) {
    if (state_ptr) {
        terminate_display(state_ptr);
        LOGI("Android platform shutdown");
    }
}

b8 platform_pump_messages() {
    // This is handled in the android_main loop
    // with ALooper_pollAll
    return true;
}

void* platform_allocate(u64 size, b8 aligned) {
    // Android doesn't require special aligned memory allocation 
    // for most use cases, so we'll ignore the aligned parameter
    return malloc(size);
}

void platform_free(void* block, b8 aligned) {
    // Same as above, ignore aligned parameter
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
    // Map your engine's color codes to Android log levels if needed
    __android_log_write(ANDROID_LOG_INFO, "YwmaaEngine", message);
}

void platform_console_write_error(const char* message, u8 color) {
    __android_log_write(ANDROID_LOG_ERROR, "YwmaaEngine", message);
}

f64 platform_get_absolute_time() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (f64)now.tv_sec + (f64)now.tv_nsec * 0.000000001;
}

void platform_sleep(u64 ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Command handler for Android app
static void handle_cmd(android_app* app, int32_t cmd) {
    PLATFORM_STATE* state = (PLATFORM_STATE*)app->userData;
    
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL) {
                state->window = app->window;
                LOGI("Android window initialized");
                
                if (state->has_focus && !state->app_initialized) {
                    if (initialize_display(state)) {
                        state->app_initialized = true;
                    }
                }
            }
            break;
            
        case APP_CMD_TERM_WINDOW:
            LOGI("Android window terminated");
            terminate_display(state);
            state->window = NULL;
            state->app_initialized = false;
            break;
            
        case APP_CMD_GAINED_FOCUS:
            LOGI("Android app gained focus");
            state->has_focus = true;
            
            if (state->window && !state->app_initialized) {
                if (initialize_display(state)) {
                    state->app_initialized = true;
                }
            }
            break;
            
        case APP_CMD_LOST_FOCUS:
            LOGI("Android app lost focus");
            state->has_focus = false;
            break;
            
        case APP_CMD_START:
            LOGI("Android app started");
            break;
            
        case APP_CMD_RESUME:
            LOGI("Android app resumed");
            break;
            
        case APP_CMD_PAUSE:
            LOGI("Android app paused");
            break;
            
        case APP_CMD_STOP:
            LOGI("Android app stopped");
            break;
            
        case APP_CMD_DESTROY:
            LOGI("Android app being destroyed");
            is_running = false;
            break;
    }
}

// Input handler for Android app
static int32_t handle_input(android_app* app, AInputEvent* event) {
    int32_t event_type = AInputEvent_getType(event);
    
    if (event_type == AINPUT_EVENT_TYPE_MOTION) {
        // Handle touch events
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        
        // Map to your engine's input system
        // input_process_touch(action, x, y);
        
        return 1; // Event handled
    } else if (event_type == AINPUT_EVENT_TYPE_KEY) {
        // Handle key events
        int32_t key_code = AKeyEvent_getKeyCode(event);
        int32_t action = AKeyEvent_getAction(event);
        
        // Map to your engine's input system
        // input_process_key(key_code, action == AKEY_EVENT_ACTION_DOWN);
        
        return 1; // Event handled
    }
    
    return 0; // Event not handled
}

// Initialize EGL display and context
static bool initialize_display(PLATFORM_STATE* state) {
    if (state->display != EGL_NO_DISPLAY) {
        // Already initialized
        return true;
    }
    
    EGLint format;
    EGLint numConfigs;
    EGLConfig config;
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };
    
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (state->display == EGL_NO_DISPLAY) {
        LOGE("Unable to get EGL display");
        return false;
    }
    
    if (!eglInitialize(state->display, NULL, NULL)) {
        LOGE("Unable to initialize EGL");
        return false;
    }
    
    if (!eglChooseConfig(state->display, attribs, &config, 1, &numConfigs) || numConfigs <= 0) {
        LOGE("Unable to choose EGL config");
        return false;
    }
    
    if (!eglGetConfigAttrib(state->display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        LOGE("Unable to get EGL config attribute");
        return false;
    }
    
    ANativeWindow_setBuffersGeometry(state->window, 0, 0, format);
    
    state->surface = eglCreateWindowSurface(state->display, config, state->window, NULL);
    if (state->surface == EGL_NO_SURFACE) {
        LOGE("Unable to create EGL surface");
        return false;
    }
    
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, contextAttribs);
    if (state->context == EGL_NO_CONTEXT) {
        LOGE("Unable to create EGL context");
        return false;
    }
    
    if (!eglMakeCurrent(state->display, state->surface, state->surface, state->context)) {
        LOGE("Unable to make EGL context current");
        return false;
    }
    
    LOGI("EGL display initialized successfully");
    return true;
}

// Terminate EGL display and context
static void terminate_display(PLATFORM_STATE* state) {
    if (state->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (state->context != EGL_NO_CONTEXT) {
            eglDestroyContext(state->display, state->context);
            state->context = EGL_NO_CONTEXT;
        }
        
        if (state->surface != EGL_NO_SURFACE) {
            eglDestroySurface(state->display, state->surface);
            state->surface = EGL_NO_SURFACE;
        }
        
        eglTerminate(state->display);
        state->display = EGL_NO_DISPLAY;
    }
}

// Android main entry point - calls engine_main
void android_main(android_app* app) {
    // Allocate memory for platform state
    PLATFORM_STATE state = {0};
    state.app = app;
    app->userData = &state;
    
    // Set up callbacks
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;
    
    LOGI("Android main starting");
    is_running = true;
    
    // Initialize the state
    state_ptr = &state;
    
    // Start the game engine in a separate thread
    // This allows the Android main thread to continue processing events
    pthread_t engine_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    // Start the engine main function
    pthread_create(&engine_thread, &attr, (void*(*)(void*))engine_main, NULL);
    pthread_attr_destroy(&attr);
    
    // Main event loop
    while (is_running) {
        int ident;
        int events;
        struct android_poll_source* source;
        
        // Process events - block when not focused
        while ((ident = ALooper_pollAll(state.has_focus ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }
            
            if (app->destroyRequested != 0) {
                LOGI("Destroy requested, shutting down");
                is_running = false;
                break;
            }
        }
        
        // Render if we have an initialized app with focus
        if (is_running && state.app_initialized && state.has_focus) {
            // Your engine should handle rendering in its main loop
            // But we still need to swap buffers here
            if (state.display != EGL_NO_DISPLAY) {
                eglSwapBuffers(state.display, state.surface);
            }
        }
        
        // Give some time back to the system when not focused
        if (!state.has_focus) {
            platform_sleep(100);
        }
    }
    
    // Clean up
    terminate_display(&state);
    LOGI("Android main finished");
}

#endif // PLATFORM_ANDROID 