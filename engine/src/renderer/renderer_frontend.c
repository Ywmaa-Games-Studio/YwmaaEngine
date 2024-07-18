#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/ymemory.h"

struct PLATFORM_STATE;

// Backend render context.
static RENDERER_BACKEND* backend = 0;

b8 renderer_init(const char* application_name, struct PLATFORM_STATE* platform_state) {
    backend = yallocate(sizeof(RENDERER_BACKEND), MEMORY_TAG_RENDERER);

    // TODO: make this configurable.
    renderer_backend_create(RENDERER_BACKEND_API_WEBGPU, platform_state, backend);
    backend->frame_number = 0;

    if (!backend->init(backend, application_name, platform_state)) {
        PRINT_ERROR("Renderer backend failed to initialize. Shutting down.");
        return FALSE;
    }

    return TRUE;
}

void renderer_shutdown() {
    backend->shutdown(backend);
    yfree(backend, sizeof(RENDERER_BACKEND), MEMORY_TAG_RENDERER);
}

b8 renderer_begin_frame(f32 delta_time) {
    return backend->begin_frame(backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time) {
    b8 result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

b8 renderer_draw_frame(RENDER_PACKET* packet) {
    // If the begin frame returned successfully, mid-frame operations may continue.
    if (renderer_begin_frame(packet->delta_time)) {

        // End the frame. If this fails, it is likely unrecoverable.
        b8 result = renderer_end_frame(packet->delta_time);

        if (!result) {
            PRINT_ERROR("renderer_end_frame failed. Application shutting down...");
            return FALSE;
        }
    }

    return TRUE;
}