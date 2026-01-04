#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/yvar.h"
#include "core/frame_data.h"
#include "core/ystring.h"
#include "math/ymath.h"

#include "resources/resource_types.h"
#include "core/systems_manager.h"

#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "systems/render_view_system.h"
typedef struct RENDERER_SYSTEM_STATE {
    RENDERER_PLUGIN plugin;
    // The number of render targets. Typically lines up with the amount of swapchain images.
    u8 window_render_target_count;
    // The current window framebuffer width.
    u32 framebuffer_width;
    // The current window framebuffer height.
    u32 framebuffer_height;

    // Indicates if the window is currently being resized.
    b8 resizing;
    // The current number of frames since the last resize operation.'
    // Only set if resizing = true. Otherwise 0.
    u8 frames_since_resize;
} RENDERER_SYSTEM_STATE;

b8 renderer_system_init(u64* memory_requirement, void* state, void* config) {
    RENDERER_SYSTEM_CONFIG* typed_config = (RENDERER_SYSTEM_CONFIG*)config;
    *memory_requirement = sizeof(RENDERER_SYSTEM_STATE);
    if (state == 0) {
        return true;
    }
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)state;

   // Default framebuffer size. Overridden when window is created.
    state_ptr->framebuffer_width = 1280;
    state_ptr->framebuffer_height = 720;
    state_ptr->resizing = false;
    state_ptr->frames_since_resize = 0;

    state_ptr->plugin = typed_config->plugin;
    state_ptr->plugin.frame_number = 0;

    RENDERER_BACKEND_CONFIG renderer_config = {0};
    renderer_config.application_name = typed_config->application_name;
    // TODO: expose this to the application to configure.
    renderer_config.flags = RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT | RENDERER_CONFIG_FLAG_POWER_SAVING_BIT;

    // Create the vsync kvar
    yvar_create_int("vsync", (renderer_config.flags & RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT) ? 1 : 0);

    // Initialize the backend.
    if (!state_ptr->plugin.init(&state_ptr->plugin, &renderer_config, &state_ptr->window_render_target_count)) {
        PRINT_ERROR("Renderer backend failed to initialize. Shutting down.");
        return false;
    }

    return true;
}

void renderer_system_shutdown(void* state) {
    if (state) {
        RENDERER_SYSTEM_STATE* typed_state = (RENDERER_SYSTEM_STATE*)state;
        typed_state->plugin.shutdown(&typed_state->plugin);
    }
}

E_RENDERER_BACKEND_API renderer_get_backend_api(void) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (state_ptr) {
        return state_ptr->plugin.backend_api;
    } else {
        PRINT_ERROR("renderer system is not initialized!");
        return RENDERER_BACKEND_API_UNKNOWN;
    }
}

void renderer_on_resized(u16 width, u16 height) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (state_ptr) {
        // Flag as resizing and store the change, but wait to regenerate.
        state_ptr->resizing = true;
        state_ptr->framebuffer_width = width;
        state_ptr->framebuffer_height = height;
        // Also reset the frame count since the last  resize operation.
        state_ptr->frames_since_resize = 0;
    } else {
        PRINT_WARNING("renderer backend does not exist to accept resize: %i %i", width, height);
    }
}

b8 renderer_draw_frame(RENDER_PACKET* packet, const struct FRAME_DATA* p_frame_data) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.frame_number++;
    // Make sure the window is not currently being resized by waiting a designated
    // number of frames after the last resize operation before performing the backend updates.
    if (state_ptr->resizing) {
        state_ptr->frames_since_resize++;

        // If the required number of frames have passed since the resize, go ahead and perform the actual updates.
        if (state_ptr->frames_since_resize >= 30) {
            f32 width = state_ptr->framebuffer_width;
            f32 height = state_ptr->framebuffer_height;
            render_view_system_on_window_resize(width, height);
            state_ptr->plugin.resized(&state_ptr->plugin, width, height);

            // Notify views of the resize.
            render_view_system_on_window_resize(width, height);

            state_ptr->frames_since_resize = 0;
            state_ptr->resizing = false;
        } else {
            // Skip rendering the frame and try again next time.
            return true;
        }
    }

    // If the begin frame returned successfully, mid-frame operations may continue.
    if (state_ptr->plugin.begin_frame(&state_ptr->plugin, p_frame_data)) {
        u8 attachment_index = state_ptr->plugin.window_attachment_index_get(&state_ptr->plugin);
        // Render each view.
        for (u32 i = 0; i < packet->view_count; ++i) {
            if (!render_view_system_on_render(packet->views[i].view, &packet->views[i], state_ptr->plugin.frame_number, attachment_index)) {
                PRINT_ERROR("Error rendering view index %i.", i);
                return false;
            }
        }

        // End the frame. If this fails, it is likely unrecoverable.
        b8 result = state_ptr->plugin.end_frame(&state_ptr->plugin, p_frame_data);

        if (!result) {
            PRINT_ERROR("renderer_end_frame failed. Application shutting down...");
            return false;
        }
    }

    return true;
}

void renderer_viewport_set(Vector4 rect) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.viewport_set(&state_ptr->plugin, rect);
}

void renderer_viewport_reset(void) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.viewport_reset(&state_ptr->plugin);
}

void renderer_scissor_set(Vector4 rect) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.scissor_set(&state_ptr->plugin, rect);
}

void renderer_scissor_reset(void) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.scissor_reset(&state_ptr->plugin);
}

void renderer_texture_create(const u8* pixels, struct TEXTURE* texture) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_create(&state_ptr->plugin, pixels, texture);
}

void renderer_texture_destroy(struct TEXTURE* texture) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (!state_ptr) {
        return;
    }
    state_ptr->plugin.texture_destroy(&state_ptr->plugin, texture);
}

void renderer_texture_create_writeable(TEXTURE* t) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_create_writeable(&state_ptr->plugin, t);
}

void renderer_texture_write_data(TEXTURE* t, u32 offset, u32 size, const u8* pixels) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_write_data(&state_ptr->plugin, t, offset, size, pixels);
}

void renderer_texture_read_data(TEXTURE* t, u32 offset, u32 size, void** out_memory) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_read_data(&state_ptr->plugin, t, offset, size, out_memory);
}

void renderer_texture_read_pixel(TEXTURE* t, u32 x, u32 y, u8** out_rgba) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_read_pixel(&state_ptr->plugin, t, x, y, out_rgba);
}

void renderer_texture_resize(TEXTURE* t, u32 new_width, u32 new_height) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_resize(&state_ptr->plugin, t, new_width, new_height);
}

b8 renderer_create_geometry(GEOMETRY* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.create_geometry(&state_ptr->plugin, geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_destroy_geometry(GEOMETRY* geometry) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.destroy_geometry(&state_ptr->plugin, geometry);
}

void renderer_draw_geometry(GEOMETRY_RENDER_DATA* data) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.draw_geometry(&state_ptr->plugin, data);
}

b8 renderer_renderpass_begin(RENDERPASS* pass, RENDER_TARGET* target) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderpass_begin(&state_ptr->plugin, pass, target);
}

b8 renderer_renderpass_end(RENDERPASS* pass) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderpass_end(&state_ptr->plugin, pass);
}

b8 renderer_shader_create(SHADER* s, const SHADER_CONFIG* config, RENDERPASS* pass, u8 stage_count, const char** stage_filenames, E_SHADER_STAGE* stages) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_create(&state_ptr->plugin, s, config, pass, stage_count, stage_filenames, stages);
}

void renderer_shader_destroy(SHADER* s) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.shader_destroy(&state_ptr->plugin, s);
}

b8 renderer_shader_init(SHADER* s) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_init(&state_ptr->plugin, s);
}

b8 renderer_shader_use(SHADER* s) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_use(&state_ptr->plugin, s);
}

b8 renderer_shader_bind_globals(SHADER* s) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_bind_globals(&state_ptr->plugin, s);
}

b8 renderer_shader_bind_instance(SHADER* s, u32 instance_id) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_bind_instance(&state_ptr->plugin, s, instance_id);
}

b8 renderer_shader_apply_globals(SHADER* s) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_apply_globals(&state_ptr->plugin, s);
}

b8 renderer_shader_apply_instance(SHADER* s, b8 needs_update) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_apply_instance(&state_ptr->plugin, s, needs_update);
}

b8 renderer_shader_acquire_instance_resources(SHADER* s, TEXTURE_MAP** maps, u32* out_instance_id) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_acquire_instance_resources(&state_ptr->plugin, s, maps, out_instance_id);
}

b8 renderer_shader_release_instance_resources(SHADER* s, u32 instance_id) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_release_instance_resources(&state_ptr->plugin, s, instance_id);
}

b8 renderer_set_uniform(SHADER* s, SHADER_UNIFORM* uniform, const void* value) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_set_uniform(&state_ptr->plugin, s, uniform, value);
}

b8 renderer_shader_after_renderpass(SHADER* s) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.shader_after_renderpass(&state_ptr->plugin, s);
}

b8 renderer_texture_map_acquire_resources(struct TEXTURE_MAP* map) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.texture_map_acquire_resources(&state_ptr->plugin, map);
}

void renderer_texture_map_release_resources(struct TEXTURE_MAP* map) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.texture_map_release_resources(&state_ptr->plugin, map);
}

void renderer_render_target_create(u8 attachment_count, RENDER_TARGET_ATTACHMENT* attachments, RENDERPASS* pass, u32 width, u32 height, RENDER_TARGET* out_target) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.render_target_create(&state_ptr->plugin, attachment_count, attachments, pass, width, height, out_target);
}

void renderer_render_target_destroy(RENDER_TARGET* target, b8 free_internal_memory) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.render_target_destroy(&state_ptr->plugin, target, free_internal_memory);

    if (free_internal_memory) {
        yzero_memory(target, sizeof(RENDER_TARGET));
    }
}

TEXTURE* renderer_window_attachment_get(u8 index) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.window_attachment_get(&state_ptr->plugin, index);
}

TEXTURE* renderer_depth_attachment_get(u8 index) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.depth_attachment_get(&state_ptr->plugin, index);
}

u8 renderer_window_attachment_index_get(void) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.window_attachment_index_get(&state_ptr->plugin);
}

u8 renderer_window_attachment_count_get(void) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.window_attachment_count_get(&state_ptr->plugin);
}

b8 renderer_renderpass_create(const RENDERPASS_CONFIG* config, RENDERPASS* out_renderpass) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (!config) {
        PRINT_ERROR("Renderpass config is required.");
        return false;
    }

    if (config->render_target_count == 0) {
        PRINT_ERROR("Cannot have a renderpass target count of 0, ya dingus.");
        return false;
    }

    out_renderpass->render_target_count = config->render_target_count;
    out_renderpass->targets = yallocate_aligned(sizeof(RENDER_TARGET) * out_renderpass->render_target_count, 8, MEMORY_TAG_ARRAY);
    out_renderpass->clear_flags = config->clear_flags;
    out_renderpass->clear_color = config->clear_color;
    out_renderpass->render_area = config->render_area;
    out_renderpass->name = string_duplicate(config->name);

    // Copy over config for each target.
    for (u32 t = 0; t < out_renderpass->render_target_count; ++t) {
        RENDER_TARGET* target = &out_renderpass->targets[t];
        target->attachment_count = config->target.attachment_count;
        target->attachments = yallocate_aligned(sizeof(RENDER_TARGET_ATTACHMENT) * target->attachment_count, 8, MEMORY_TAG_ARRAY);

        // Each attachment for the target.
        for (u32 a = 0; a < target->attachment_count; ++a) {
            RENDER_TARGET_ATTACHMENT* attachment = &target->attachments[a];
            RENDER_TARGET_ATTACHMENT_CONFIG* attachment_config = &config->target.attachments[a];

            attachment->source = attachment_config->source;
            attachment->type = attachment_config->type;
            attachment->load_operation = attachment_config->load_operation;
            attachment->store_operation = attachment_config->store_operation;
            attachment->texture = 0;
        }
    }

    return state_ptr->plugin.renderpass_create(&state_ptr->plugin, config, out_renderpass);
}

void renderer_renderpass_destroy(RENDERPASS* pass) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    // Destroy its rendertargets.
    for (u32 i = 0; i < pass->render_target_count; ++i) {
        renderer_render_target_destroy(&pass->targets[i], true);
    }

    if (pass->name) {
        yfree(pass->name);
        pass->name = 0;
    }
    
    state_ptr->plugin.renderpass_destroy(&state_ptr->plugin, pass);
}

b8 renderer_is_multithreaded(void) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.is_multithreaded(&state_ptr->plugin);
}

b8 renderer_flag_enabled(RENDERER_CONFIG_FLAGS flag) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.flag_enabled(&state_ptr->plugin, flag);
}

void renderer_flag_set_enabled(RENDERER_CONFIG_FLAGS flag, b8 enabled) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.flag_set_enabled(&state_ptr->plugin, flag, enabled);
}


b8 renderer_renderbuffer_create(E_RENDERBUFFER_TYPE type, u64 total_size, b8 use_freelist, RENDER_BUFFER* out_buffer) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (!out_buffer) {
        PRINT_ERROR("renderer_renderbuffer_create requires a valid pointer to hold the created buffer.");
        return false;
    }

    yzero_memory(out_buffer, sizeof(RENDER_BUFFER));

    out_buffer->type = type;
    out_buffer->total_size = total_size;

    // Create the freelist, if needed.
    if (use_freelist) {
        freelist_create(total_size, &out_buffer->freelist_memory_requirement, 0, 0);
        out_buffer->freelist_block = yallocate_aligned(out_buffer->freelist_memory_requirement, 8, MEMORY_TAG_RENDERER);
        freelist_create(total_size, &out_buffer->freelist_memory_requirement, out_buffer->freelist_block, &out_buffer->buffer_freelist);
    }

    // Create the internal buffer from the backend.
    if (!state_ptr->plugin.renderbuffer_create_internal(&state_ptr->plugin, out_buffer)) {
        PRINT_ERROR("Unable to create backing buffer for RENDER_BUFFER. Application cannot continue.");
        return false;
    }

    return true;
}

void renderer_renderbuffer_destroy(RENDER_BUFFER* buffer) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (buffer) {
        if (buffer->freelist_memory_requirement > 0) {
            freelist_destroy(&buffer->buffer_freelist);
            yfree(buffer->freelist_block);
            buffer->freelist_memory_requirement = 0;
        }

        // Free up the backend resources.
        state_ptr->plugin.renderbuffer_destroy_internal(&state_ptr->plugin, buffer);
        buffer->internal_data = 0;
    }
}

b8 renderer_renderbuffer_bind(RENDER_BUFFER* buffer, u64 offset) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    if (!buffer) {
        PRINT_ERROR("renderer_renderbuffer_bind requires a valid pointer to a buffer.");
        return false;
    }

    return state_ptr->plugin.renderbuffer_bind(&state_ptr->plugin, buffer, offset);
}

b8 renderer_renderbuffer_unbind(RENDER_BUFFER* buffer) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_unbind(&state_ptr->plugin, buffer);
}

void* renderer_renderbuffer_map_memory(RENDER_BUFFER* buffer, u64 offset, u64 size) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_map_memory(&state_ptr->plugin, buffer, offset, size);
}

void renderer_renderbuffer_unmap_memory(RENDER_BUFFER* buffer, u64 offset, u64 size) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    state_ptr->plugin.renderbuffer_unmap_memory(&state_ptr->plugin, buffer, offset, size);
}

b8 renderer_renderbuffer_flush(RENDER_BUFFER* buffer, u64 offset, u64 size) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_flush(&state_ptr->plugin, buffer, offset, size);
}

b8 renderer_renderbuffer_read(RENDER_BUFFER* buffer, u64 offset, u64 size, void** out_memory) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_read(&state_ptr->plugin, buffer, offset, size, out_memory);
}

b8 renderer_renderbuffer_resize(RENDER_BUFFER* buffer, u64 new_total_size) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    // Sanity check.
    if (new_total_size <= buffer->total_size) {
        PRINT_ERROR("renderer_renderbuffer_resize requires that new size be larger than the old. Not doing this could lead to data loss.");
        return false;
    }

    if (buffer->freelist_memory_requirement > 0) {
        // Resize the freelist first, if used.
        u64 new_memory_requirement = 0;
        freelist_resize(&buffer->buffer_freelist, &new_memory_requirement, 0, 0, 0);
        void* new_block = yallocate_aligned(new_memory_requirement, 8, MEMORY_TAG_RENDERER);
        void* old_block = 0;
        if (!freelist_resize(&buffer->buffer_freelist, &new_memory_requirement, new_block, new_total_size, &old_block)) {
            PRINT_ERROR("renderer_renderbuffer_resize failed to resize internal free list.");
            yfree(new_block);
            return false;
        }

        // Clean up the old memory, then assign the new properties over.
        yfree(old_block);
        buffer->freelist_memory_requirement = new_memory_requirement;
        buffer->freelist_block = new_block;
    }

    b8 result = state_ptr->plugin.renderbuffer_resize(&state_ptr->plugin, buffer, new_total_size);
    if (result) {
        buffer->total_size = new_total_size;
    } else {
        PRINT_ERROR("Failed to resize internal RENDER_BUFFER resources.");
    }
    return result;
}

b8 renderer_renderbuffer_allocate(RENDER_BUFFER* buffer, u64 size, u64* out_offset) {
    if (!buffer || !size || !out_offset) {
        PRINT_ERROR("renderer_renderbuffer_allocate requires valid buffer, a nonzero size and valid pointer to hold offset.");
        return false;
    }

    if (buffer->freelist_memory_requirement == 0) {
        PRINT_WARNING("renderer_renderbuffer_allocate called on a buffer not using freelists. Offset will not be valid. Call renderer_renderbuffer_load_range instead.");
        *out_offset = 0;
        return true;
    }
    return freelist_allocate_block(&buffer->buffer_freelist, size, out_offset);
}

b8 renderer_renderbuffer_free(RENDER_BUFFER* buffer, u64 size, u64 offset) {
    if (!buffer || !size) {
        PRINT_ERROR("renderer_renderbuffer_free requires valid buffer and a nonzero size.");
        return false;
    }

    if (buffer->freelist_memory_requirement == 0) {
        PRINT_WARNING("renderer_renderbuffer_free called on a buffer not using freelists. Nothing was done.");
        return true;
    }
    return freelist_free_block(&buffer->buffer_freelist, size, offset);
}

b8 renderer_renderbuffer_load_range(RENDER_BUFFER* buffer, u64 offset, u64 size, const void* data) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_load_range(&state_ptr->plugin, buffer, offset, size, data);
}

b8 renderer_renderbuffer_copy_range(RENDER_BUFFER* source, u64 source_offset, RENDER_BUFFER* dest, u64 dest_offset, u64 size) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_copy_range(&state_ptr->plugin, source, source_offset, dest, dest_offset, size);
}

b8 renderer_renderbuffer_draw(RENDER_BUFFER* buffer, u64 offset, u32 element_count, b8 bind_only) {
    RENDERER_SYSTEM_STATE* state_ptr = (RENDERER_SYSTEM_STATE*)systems_manager_get_state(Y_SYSTEM_TYPE_RENDERER);
    return state_ptr->plugin.renderbuffer_draw(&state_ptr->plugin, buffer, offset, element_count, bind_only);
}
