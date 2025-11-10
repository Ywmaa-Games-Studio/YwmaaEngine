#include "camera_system.h"

#include "core/logger.h"
#include "core/ystring.h"
#include "data_structures/hashtable.h"
#include "renderer/camera.h"

typedef struct CAMERA_LOOKUP {
    u16 id;
    u16 reference_count;
    Camera c;
} CAMERA_LOOKUP;

typedef struct CAMERA_SYSTEM_STATE {
    CAMERA_SYSTEM_CONFIG config;
    HASHTABLE lookup;
    void* hashtable_block;
    CAMERA_LOOKUP* cameras;

    // A default, non-registered Camera that always exists as a fallback.
    Camera default_camera;
} CAMERA_SYSTEM_STATE;

static CAMERA_SYSTEM_STATE* state_ptr;

b8 camera_system_init(u64* memory_requirement, void* state, void* config) {
    CAMERA_SYSTEM_CONFIG* typed_config = (CAMERA_SYSTEM_CONFIG*)config;
    if (typed_config->max_camera_count == 0) {
        PRINT_ERROR("camera_system_init - typed_config->max_camera_count must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 struct_requirement = sizeof(CAMERA_SYSTEM_STATE);
    u64 array_requirement = sizeof(CAMERA_LOOKUP) * typed_config->max_camera_count;
    u64 hashtable_requirement = sizeof(CAMERA_LOOKUP) * typed_config->max_camera_count;
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

    if (!state) {
        return true;
    }

    state_ptr = (CAMERA_SYSTEM_STATE*)state;
    state_ptr->config = *typed_config;

    // The array block is after the state. Already allocated, so just set the pointer.
    void* array_block = state + struct_requirement;
    state_ptr->cameras = array_block;

    // Hashtable block is after array.
    void* hashtable_block = array_block + array_requirement;

    // Create a hashtable for Camera lookups.
    hashtable_create(sizeof(u16), typed_config->max_camera_count, hashtable_block, false, &state_ptr->lookup);

    // Fill the hashtable with invalid references to use as a default.
    u16 invalid_id = INVALID_ID_U16;
    hashtable_fill(&state_ptr->lookup, &invalid_id);

    // Invalidate all cameras in the array.
    u32 count = state_ptr->config.max_camera_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->cameras[i].id = INVALID_ID_U16;
        state_ptr->cameras[i].reference_count = 0;
    }

    // Setup default Camera.
    state_ptr->default_camera = camera_create();

    return true;
}

void camera_system_shutdown(void* state) {
    CAMERA_SYSTEM_STATE* s = (CAMERA_SYSTEM_STATE*)state;
    if (s) {
        // NOTE: If cameras need to be destroyed, do it here.
        // // Invalidate all cameras in the array.
        // u32 count = s->config.max_camera_count;
        // for (u32 i = 0; i < count; ++i) {
        //     if (s->cameras[i].id != INVALID_ID) {

        //     }
        // }
    }

    state_ptr = 0;
}

Camera* camera_system_acquire(const char* name) {
    if (state_ptr) {
        if (strings_equali(name, DEFAULT_CAMERA_NAME)) {
            return &state_ptr->default_camera;
        }
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->lookup, name, &id)) {
            PRINT_ERROR("camera_system_acquire failed lookup. Null returned.");
            return 0;
        }

        if (id == INVALID_ID_U16) {
            // Find free slot
            for (u16 i = 0; i < state_ptr->config.max_camera_count; ++i) {
                if (i == INVALID_ID_U16) {
                    id = i;
                    break;
                }
            }
            if (id == INVALID_ID_U16) {
                PRINT_ERROR("camera_system_acquire failed to acquire new slot. Adjust Camera system config to allow more. Null returned.");
                return 0;
            }

            // Create/register the new Camera.
            PRINT_TRACE("Creating new Camera named '%s'...");
            state_ptr->cameras[id].c = camera_create();
            state_ptr->cameras[id].id = id;

            // Update the hashtable.
            hashtable_set(&state_ptr->lookup, name, &id);
        }
        state_ptr->cameras[id].reference_count++;
        return &state_ptr->cameras[id].c;
    }

    PRINT_ERROR("camera_system_acquire called before system initialzation. Null returned.");
    return 0;
}

void camera_system_release(const char* name) {
    if (state_ptr) {
        if (strings_equali(name, DEFAULT_CAMERA_NAME)) {
            PRINT_TRACE("Cannot release default Camera. Nothing was done.");
            return;
        }
        u16 id = INVALID_ID_U16;
        if (!hashtable_get(&state_ptr->lookup, name, &id)) {
            PRINT_WARNING("camera_system_release failed lookup. Nothing was done.");
        }

        if (id != INVALID_ID_U16) {
            // Decrement the reference count, and reset the Camera if the counter reaches 0.
            state_ptr->cameras[id].reference_count--;
            if (state_ptr->cameras[id].reference_count < 1) {
                camera_reset(&state_ptr->cameras[id].c);
                state_ptr->cameras[id].id = INVALID_ID_U16;
                hashtable_set(&state_ptr->lookup, name, &state_ptr->cameras[id].id);
            }
        }
    }
}

Camera* camera_system_get_default(void) {
    if (state_ptr) {
        return &state_ptr->default_camera;
    }

    PRINT_ERROR("camera_system_get_default called before system was initialized. Returned null.");
    return 0;
}
