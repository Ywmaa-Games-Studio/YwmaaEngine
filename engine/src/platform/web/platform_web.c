/* PLATFORM WEB.c
 *   by Youssef Abdelkareem (ywmaa)
 *
 * Created:
 *   2025.04.15 -02:05
 * Last edited:
 *   2026.02.08 -09:22
 * Auto updated?
 *   Yes
 *
 * Description:
 *   Web Platform Layer
**/

#include "../platform.h"

// Web platform layer
#if defined(YPLATFORM_WEB)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
#include <emscripten/threading.h>
#include "core/logger.h"
#include "core/event.h"
#include "input/input.h"
#include "data_structures/darray.h"
#include "core/ymemory.h"
#include "core/ystring.h"
#include <dlfcn.h>
#include "core/ythread.h"
#include "core/ymutex.h"

const char* web_canvas_name = "#RenderingSurface";
typedef struct PLATFORM_STATE {
    i32 width;
    i32 height;
} PLATFORM_STATE;

static PLATFORM_STATE *state_ptr;
E_KEYS translate_keycode(DOM_PK_CODE_TYPE keycode);
typedef enum E_WEB_KEY_STATE {
    WEB_KEY_PRESSED = 0x02,
    WEB_KEY_DOWN = 0x03,
    WEB_KEY_UP = 0x04
} E_WEB_KEY_STATE;
//static inline const char *emscripten_event_type_to_string(int eventType) {
//  const char *events[] = { "(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize",
//    "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange",
//    "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload",
//    "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "mouseenter", "mouseleave", "mouseover", "mouseout", "(invalid)" };
//  ++eventType;
//  if (eventType < 0) eventType = 0;
//  if (eventType >= sizeof(events)/sizeof(events[0])) eventType = sizeof(events)/sizeof(events[0])-1;
//  return events[eventType];
//}

int interpret_charcode_for_keyevent(int eventType, const EmscriptenKeyboardEvent *e) {
  // Only KeyPress events carry a charCode. For KeyDown and KeyUp events, these don't seem to be present yet, until later when the KeyDown
  // is transformed to KeyPress. Sometimes it can be useful to already at KeyDown time to know what the charCode of the resulting
  // KeyPress will be. The following attempts to do this:
  if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && e->which) return e->which;
  if (e->charCode) return e->charCode;
  if (strlen(e->key) == 1) return (int)e->key[0];
  if (e->which) return e->which;
  return e->keyCode;
}

int number_of_characters_in_utf8_string(const char *str) {
  if (!str) return 0;
  int num_chars = 0;
  while (*str) {
    if ((*str++ & 0xC0) != 0x80) ++num_chars; // Skip all continuation bytes
  }
  return num_chars;
}

int emscripten_key_event_is_printable_character(const EmscriptenKeyboardEvent *keyEvent) {
  // Not sure if this is correct, but heuristically looks good. Improvements on corner cases welcome.
  return number_of_characters_in_utf8_string(keyEvent->key) == 1;
}

static EM_BOOL mouse_move_callback(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    // e->targetX/Y are relative to the canvas element
    input_process_mouse_move((i32)e->targetX, (i32)e->targetY);
    return EM_TRUE;
}

static EM_BOOL emscripten_mouse_button_callback(int eventType,const EmscriptenMouseEvent* e,void* userData) {
    b8 pressed = (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN);
    E_BUTTONS mouse_button = BUTTON_MAX_BUTTONS;

    switch (e->button) {
        case 0:
            mouse_button = BUTTON_LEFT;
            break;
        case 1:
            mouse_button = BUTTON_MIDDLE;
            break;
        case 2:
            mouse_button = BUTTON_RIGHT;
            break;
    }

    input_process_button(mouse_button, pressed);
    return EM_TRUE;
}

static EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent *e, void *userData) {
    // e->deltaY is the vertical scroll amount. 
    // Invert it to match typical Windows behavior (scroll up is positive)
    i32 z_delta = 0;
    if (e->deltaY > 0) {
        z_delta = -1;
    } else if (e->deltaY < 0) {
        z_delta = 1;
    }

    if (z_delta != 0) {
        input_process_mouse_wheel(z_delta);
    }
    
    // Return EM_TRUE to prevent the browser from scrolling the whole page
    return EM_TRUE; 
}

bool key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    int dom_pk_code = emscripten_compute_dom_pk_code(e->code);
    E_KEYS key = translate_keycode(dom_pk_code);
    b8 pressed = (eventType == EMSCRIPTEN_EVENT_KEYDOWN);
    // Pass to the input subsystem for processing.
    input_process_key(key, pressed);
  //printf("%s, key: \"%s\" (printable: %s), code: \"%s\" = %s (%d), location: %i,%s%s%s%s repeat: %d, locale: \"%s\", char: \"%s\", charCode: %u (interpreted: %d), keyCode: %s(%u), which: %u\n",
  //  emscripten_event_type_to_string(eventType), e->key, emscripten_key_event_is_printable_character(e) ? "true" : "false", e->code,
  //  emscripten_dom_pk_code_to_string(dom_pk_code), dom_pk_code, e->location,
  //  e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
  //  e->repeat, e->locale, e->charValue, e->charCode, interpret_charcode_for_keyevent(eventType, e), emscripten_dom_vk_to_string(e->keyCode), e->keyCode, e->which);

    return 0;
}

bool resize_callback(int eventType, const EmscriptenUiEvent *e, void *userData) {
    //printf("%s, detail: %d, document.body.client size: (%d,%d), window.inner size: (%d,%d), scrollPos: (%d, %d)\n",
    //emscripten_event_type_to_string(eventType), e->detail, e->documentBodyClientWidth, e->documentBodyClientHeight,
    //e->windowInnerWidth, e->windowInnerHeight, e->scrollTop, e->scrollLeft);

    EVENT_CONTEXT context;
    context.data.u16[0] = e->documentBodyClientWidth;
    context.data.u16[1] = e->documentBodyClientHeight;
    event_fire(EVENT_CODE_RESIZED, 0, context);

  return 0;
}

b8 platform_system_startup(u64 *memory_requirement, void *state, void *config) {
    PLATFORM_SYSTEM_CONFIG* typed_config = (PLATFORM_SYSTEM_CONFIG*)config;
    *memory_requirement = sizeof(PLATFORM_STATE);
    if (state == 0) {
        return true;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
    // For testing purposes, rename the canvas on the page to some arbitrary ID.
    EM_ASM(document.getElementById('canvas').id = "RenderingSurface");
#pragma clang diagnostic pop
    // in web it is started already
    state_ptr = state;
    // Get the canvas element
    PRINT_DEBUG("Web Platform system startup");
    emscripten_set_canvas_element_size(web_canvas_name, typed_config->width, typed_config->height);
    EMSCRIPTEN_RESULT result = emscripten_get_canvas_element_size(web_canvas_name, &state_ptr->width, &state_ptr->height);
    PRINT_INFO("canvas width: %i, height: %i", state_ptr->width, state_ptr->height);
    if (result != EMSCRIPTEN_RESULT_SUCCESS) {
        return false;
    }

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_TRUE, key_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_TRUE, key_callback);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, false, resize_callback);

    // Use the canvas name for movement so coordinates are relative to the element
    emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_TRUE, mouse_move_callback);
    
    // Wheel events are usually better captured on the window or canvas
    emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_TRUE, wheel_callback);

    emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, emscripten_mouse_button_callback);

    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL,EM_TRUE, emscripten_mouse_button_callback);
    
    // Fire initial resize event
    EVENT_CONTEXT context;
    context.data.u16[0] = state_ptr->width;
    context.data.u16[1] = state_ptr->height;
    event_fire(EVENT_CODE_RESIZED, 0, context);
    return true;
}

void platform_system_shutdown(void* platform_state){
    
}

b8 platform_pump_messages(void){
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

void platform_console_write(const char* message, u8 colour) {
    // ERROR,WARN,INFO,DEBUG,TRACE
    const char* colour_strings[] = {"1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

f64 platform_get_absolute_time(void){
    return emscripten_get_now() * 0.001;
}

void platform_sleep(u64 ms){
    PRINT_WARNING("platform_sleep is not supported!");
    //emscripten_sleep(ms);
}

i32 platform_get_processor_count(void) {
    // Returns the number of logical processors (navigator.hardwareConcurrency)
    return emscripten_num_logical_cores();
}

// Key translation
E_KEYS translate_keycode(DOM_PK_CODE_TYPE keycode) {
    switch (keycode) {
        case DOM_PK_BACKSPACE:
            return KEY_BACKSPACE;
        case DOM_PK_ENTER:
            return KEY_ENTER;
        case DOM_PK_TAB:
            return KEY_TAB;
            //case XK_Shift: return KEY_SHIFT;
            //case XK_Control: return KEY_CONTROL;

        case DOM_PK_PAUSE:
            return KEY_PAUSE;
        case DOM_PK_CAPS_LOCK:
            return KEY_CAPITAL;

        case DOM_PK_ESCAPE:
            return KEY_ESCAPE;

            // Not supported
            // case : return KEY_CONVERT;
            // case : return KEY_NONCONVERT;
            // case : return KEY_ACCEPT;

        //case XK_Mode_switch:
        //    return KEY_MODECHANGE;

        case DOM_PK_SPACE:
            return KEY_SPACE;
        //case XK_Prior:
        //    return KEY_PRIOR;
        //case XK_Next:
        //    return KEY_NEXT;
        case DOM_PK_END:
            return KEY_END;
        case DOM_PK_HOME:
            return KEY_HOME;
        case DOM_PK_ARROW_LEFT:
            return KEY_LEFT;
        case DOM_PK_ARROW_UP:
            return KEY_UP;
        case DOM_PK_ARROW_RIGHT:
            return KEY_RIGHT;
        case DOM_PK_ARROW_DOWN:
            return KEY_DOWN;
        case DOM_PK_MEDIA_SELECT:
            return KEY_SELECT;
        case DOM_PK_PRINT_SCREEN:
            return KEY_PRINT;
        //case XK_Execute:
        //    return KEY_EXECUTE;
        // case XK_snapshot: return KEY_SNAPSHOT; // not supported
        case DOM_PK_INSERT:
            return KEY_INSERT;
        case DOM_PK_DELETE:
            return KEY_DELETE;
        case DOM_PK_HELP:
            return KEY_HELP;

        case DOM_PK_META_LEFT:
            return KEY_LSUPER;
        case DOM_PK_META_RIGHT:
            return KEY_RSUPER;
            // case XK_apps: return KEY_APPS; // not supported

            // case XK_sleep: return KEY_SLEEP; //not supported

        case DOM_PK_NUMPAD_0:
            return KEY_NUMPAD0;
        case DOM_PK_NUMPAD_1:
            return KEY_NUMPAD1;
        case DOM_PK_NUMPAD_2:
            return KEY_NUMPAD2;
        case DOM_PK_NUMPAD_3:
            return KEY_NUMPAD3;
        case DOM_PK_NUMPAD_4:
            return KEY_NUMPAD4;
        case DOM_PK_NUMPAD_5:
            return KEY_NUMPAD5;
        case DOM_PK_NUMPAD_6:
            return KEY_NUMPAD6;
        case DOM_PK_NUMPAD_7:
            return KEY_NUMPAD7;
        case DOM_PK_NUMPAD_8:
            return KEY_NUMPAD8;
        case DOM_PK_NUMPAD_9:
            return KEY_NUMPAD9;
        case DOM_PK_NUMPAD_MULTIPLY:
            return KEY_MULTIPLY;
        case DOM_PK_NUMPAD_ADD:
            return KEY_ADD;
        //case XK_KP_Separator:
        //    return KEY_SEPARATOR;
        case DOM_PK_NUMPAD_SUBTRACT:
            return KEY_SUBTRACT;
        case DOM_PK_NUMPAD_DECIMAL:
            return KEY_DECIMAL;
        case DOM_PK_NUMPAD_DIVIDE:
            return KEY_DIVIDE;
        case DOM_PK_F1:
            return KEY_F1;
        case DOM_PK_F2:
            return KEY_F2;
        case DOM_PK_F3:
            return KEY_F3;
        case DOM_PK_F4:
            return KEY_F4;
        case DOM_PK_F5:
            return KEY_F5;
        case DOM_PK_F6:
            return KEY_F6;
        case DOM_PK_F7:
            return KEY_F7;
        case DOM_PK_F8:
            return KEY_F8;
        case DOM_PK_F9:
            return KEY_F9;
        case DOM_PK_F10:
            return KEY_F10;
        case DOM_PK_F11:
            return KEY_F11;
        case DOM_PK_F12:
            return KEY_F12;
        case DOM_PK_F13:
            return KEY_F13;
        case DOM_PK_F14:
            return KEY_F14;
        case DOM_PK_F15:
            return KEY_F15;
        case DOM_PK_F16:
            return KEY_F16;
        case DOM_PK_F17:
            return KEY_F17;
        case DOM_PK_F18:
            return KEY_F18;
        case DOM_PK_F19:
            return KEY_F19;
        case DOM_PK_F20:
            return KEY_F20;
        case DOM_PK_F21:
            return KEY_F21;
        case DOM_PK_F22:
            return KEY_F22;
        case DOM_PK_F23:
            return KEY_F23;
        case DOM_PK_F24:
            return KEY_F24;

        case DOM_PK_NUM_LOCK:
            return KEY_NUMLOCK;
        case DOM_PK_SCROLL_LOCK:
            return KEY_SCROLL;

        case DOM_PK_NUMPAD_EQUAL:
            return KEY_NUMPAD_EQUAL;

        case DOM_PK_SHIFT_LEFT:
            return KEY_LSHIFT;
        case DOM_PK_SHIFT_RIGHT:
            return KEY_RSHIFT;
        case DOM_PK_CONTROL_LEFT:
            return KEY_LCONTROL;
        case DOM_PK_CONTROL_RIGHT:
            return KEY_RCONTROL;
        case DOM_PK_ALT_LEFT:
            return KEY_LALT;
        case DOM_PK_ALT_RIGHT:
            return KEY_RALT;

        case DOM_PK_SEMICOLON:
            return KEY_SEMICOLON;
        //case XK_plus:
        //    return KEY_PLUS;
        case DOM_PK_COMMA:
            return KEY_COMMA;
        case DOM_PK_MINUS:
            return KEY_MINUS;
        case DOM_PK_PERIOD:
            return KEY_PERIOD;
        case DOM_PK_SLASH:
            return KEY_SLASH;
        //case XK_grave:
        //    return KEY_GRAVE;

        case DOM_PK_A:
            return KEY_A;
        case DOM_PK_B:
            return KEY_B;
        case DOM_PK_C:
            return KEY_C;
        case DOM_PK_D:
            return KEY_D;
        case DOM_PK_E:
            return KEY_E;
        case DOM_PK_F:
            return KEY_F;
        case DOM_PK_G:
            return KEY_G;
        case DOM_PK_H:
            return KEY_H;
        case DOM_PK_I:
            return KEY_I;
        case DOM_PK_J:
            return KEY_J;
        case DOM_PK_K:
            return KEY_K;
        case DOM_PK_L:
            return KEY_L;
        case DOM_PK_M:
            return KEY_M;
        case DOM_PK_N:
            return KEY_N;
        case DOM_PK_O:
            return KEY_O;
        case DOM_PK_P:
            return KEY_P;
        case DOM_PK_Q:
            return KEY_Q;
        case DOM_PK_R:
            return KEY_R;
        case DOM_PK_S:
            return KEY_S;
        case DOM_PK_T:
            return KEY_T;
        case DOM_PK_U:
            return KEY_U;
        case DOM_PK_V:
            return KEY_V;
        case DOM_PK_W:
            return KEY_W;
        case DOM_PK_X:
            return KEY_X;
        case DOM_PK_Y:
            return KEY_Y;
        case DOM_PK_Z:
            return KEY_Z;

        default:
            return 0;
    }
}

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

// Assuming these types are defined in your headers:
// b8, i32, u64, PFN_THREAD_START, YTHREAD, YMUTEX

// -------------------------------------------------------------------------
// THREADS
// -------------------------------------------------------------------------

b8 ythread_create(PFN_THREAD_START start_function_ptr, void* params, b8 auto_detach, YTHREAD* out_thread) {
    if (!start_function_ptr || !out_thread) {
        return false;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (auto_detach) {
        // Better than manual detach: the thread cleans itself up immediately on exit.
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    // Emscripten uses a 32-bit integer for pthread_t in wasm32
    i32 result = pthread_create((pthread_t*)&out_thread->thread_id, &attr, (void* (*)(void*))start_function_ptr, params);
    pthread_attr_destroy(&attr);

    if (result != 0) {
        switch (result) {
            case EAGAIN:
                PRINT_ERROR("Failed to create thread: insufficient resources (check -s PTHREAD_POOL_SIZE).");
                break;
            default:
                PRINT_ERROR("Failed to create thread: errno=%i", result);
                break;
        }
        return false;
    }

    PRINT_DEBUG("Starting process on thread id: %lu", (unsigned long)out_thread->thread_id);

    // If joinable, we store the ID in internal_data so we can join/cancel later.
    if (!auto_detach) {
        out_thread->internal_data = platform_allocate(sizeof(pthread_t), false);
        *(pthread_t*)out_thread->internal_data = (pthread_t)out_thread->thread_id;
    } else {
        out_thread->internal_data = 0;
    }

    return true;
}

void ythread_destroy(YTHREAD* thread) {
    // In Emscripten, "destroying" a thread usually means joining or canceling.
    // If it's joinable, we should cancel and free.
    ythread_cancel(thread);
}

void ythread_detach(YTHREAD* thread) {
    if (thread && thread->internal_data) {
        pthread_t t = *(pthread_t*)thread->internal_data;
        i32 result = pthread_detach(t);
        if (result != 0) {
            PRINT_ERROR("Failed to detach thread. errno=%i", result);
        }
        platform_free(thread->internal_data, false);
        thread->internal_data = 0;
    }
}

void ythread_cancel(YTHREAD* thread) {
    if (thread && thread->internal_data) {
        pthread_t t = *(pthread_t*)thread->internal_data;
        // Note: Emscripten support for pthread_cancel can be limited depending on sync points.
        pthread_cancel(t);
        
        platform_free(thread->internal_data, false);
        thread->internal_data = 0;
        thread->thread_id = 0;
    }
}

b8 ythread_is_active(YTHREAD* thread) {
    if (!thread) return false;
    // For joinable threads, internal_data exists. 
    // For detached threads, we check if the ID is non-zero (though this is less reliable).
    return thread->internal_data != 0 || thread->thread_id != 0;
}

void ythread_sleep(YTHREAD* thread, u64 ms) {
    double start = emscripten_get_now();
    while (emscripten_get_now() - start < (double)ms) {
        // Do nothing, just loop.
        // This does NOT require Asyncify/JSPI.
    }
}

u64 platform_current_thread_id(void) {
    return (u64)pthread_self();
}

// -------------------------------------------------------------------------
// MUTEXES
// -------------------------------------------------------------------------

b8 ymutex_create(YMUTEX* out_mutex) {
    if (!out_mutex) return false;

    pthread_mutex_t* mutex = platform_allocate(sizeof(pthread_mutex_t), false);
    
    // Using default attributes. For recursion, use pthread_mutexattr_settype.
    i32 result = pthread_mutex_init(mutex, 0);
    if (result != 0) {
        PRINT_ERROR("Mutex creation failure! errno=%i", result);
        platform_free(mutex, false);
        return false;
    }

    out_mutex->internal_data = mutex;
    return true;
}

void ymutex_destroy(YMUTEX* mutex) {
    if (mutex && mutex->internal_data) {
        i32 result = pthread_mutex_destroy((pthread_mutex_t*)mutex->internal_data);
        if (result == EBUSY) {
            PRINT_ERROR("Unable to destroy mutex: mutex is locked.");
        }
        platform_free(mutex->internal_data, false);
        mutex->internal_data = 0;
    }
}

b8 ymutex_lock(YMUTEX* mutex) {
    if (!mutex || !mutex->internal_data) return false;
    
    i32 result = pthread_mutex_lock((pthread_mutex_t*)mutex->internal_data);
    if (result != 0) {
        // EDEADLK is common in Emscripten if you try to lock on the main thread 
        // while a worker holds the lock.
        PRINT_ERROR("Mutex lock failed: errno=%i", result);
        return false;
    }
    return true;
}

b8 ymutex_unlock(YMUTEX* mutex) {
    if (!mutex || !mutex->internal_data) return false;
    
    i32 result = pthread_mutex_unlock((pthread_mutex_t*)mutex->internal_data);
    if (result != 0) {
        PRINT_ERROR("Mutex unlock failed: errno=%i", result);
        return false;
    }
    return true;
}

const char* platform_dynamic_library_extension(void) {
    return ".wasm";
}

const char* platform_dynamic_library_prefix(void) {
    return "";
}

b8 platform_dynamic_library_load(const char *name, DYNAMIC_LIBRARY *out_library) {
    if (!out_library) {
        return false;
    }
    yzero_memory(out_library, sizeof(DYNAMIC_LIBRARY));
    if (!name) {
        return false;
    }

    char filename[260];  // NOTE: same as Windows, for now.
    yzero_memory(filename, sizeof(char) * 260);

    const char *extension = platform_dynamic_library_extension();
    const char *prefix = platform_dynamic_library_prefix();

    string_format(filename, "%s%s%s", prefix, name, extension);

    void* library = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);  // "libtestbed_lib_loaded.dylib"

    // try ../lib/ path
    if (!library) {
        yzero_memory(filename, sizeof(char) * 260);
        string_format(filename, "../lib/%s%s%s", prefix, name, extension);
        library = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);  // "libtestbed_lib_loaded.dylib"
    }
    if (!library) {
        PRINT_ERROR("Error opening library: %s", dlerror());
        return false;
    }

    out_library->name = string_duplicate(name);
    out_library->filename = string_duplicate(filename);

    out_library->internal_data_size = 8;
    out_library->internal_data = library;

    out_library->functions = darray_create(DYNAMIC_LIBRARY_FUNCTION);

    return true;
}

b8 platform_dynamic_library_unload(DYNAMIC_LIBRARY *library) {
    if (!library) {
        return false;
    }

    if (!library->internal_data) {
        return false;
    }

    i32 result = dlclose(library->internal_data);
    if (result != 0) {  // Opposite of Windows, 0 means success.
        return false;
    }

    library->internal_data = 0;

    if (library->name) {
        yfree((void *)library->name);
    }

    if (library->filename) {
        yfree((void *)library->filename);
    }

    if (library->functions) {
        u32 count = darray_length(library->functions);
        for (u32 i = 0; i < count; ++i) {
            DYNAMIC_LIBRARY_FUNCTION *f = &library->functions[i];
            if (f->name) {
                yfree((void *)f->name);
            }
        }

        darray_destroy(library->functions);
        library->functions = 0;
    }

    yzero_memory(library, sizeof(DYNAMIC_LIBRARY));

    return true;
}

b8 platform_dynamic_library_load_function(const char *name, DYNAMIC_LIBRARY *library) {
    if (!name || !library) {
        return false;
    }

    if (!library->internal_data) {
        return false;
    }

    void *f_addr = dlsym(library->internal_data, name);
    if (!f_addr) {
        return false;
    }

    DYNAMIC_LIBRARY_FUNCTION f = {0};
    f.pfn = f_addr;
    f.name = string_duplicate(name);
    darray_push(library->functions, f);

    return true;
}

E_PLATFORM_ERROR_CODE platform_copy_file(const char *source, const char *dest, b8 overwrite_if_exists) {
    return PLATFORM_ERROR_UNSUPPORTED;
}

b8 platform_watch_file(const char *file_path, u32 *out_watch_id) {
    if (out_watch_id) *out_watch_id = INVALID_ID;
    return false;
}

b8 platform_unwatch_file(u32 watch_id) {
    return false;
}

void platform_update_watches(void) {
    
}

#endif // defined(YPLATFORM_WEB)
