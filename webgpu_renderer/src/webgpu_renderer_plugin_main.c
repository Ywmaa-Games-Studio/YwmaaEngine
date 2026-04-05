#include "webgpu_renderer_plugin_main.h"

#include "renderer/webgpu/webgpu_backend.h"
#include <core/ymemory.h>

b8 plugin_create(RENDERER_PLUGIN* out_renderer_backend) {
    out_renderer_backend->backend_api = RENDERER_BACKEND_API_WEBGPU;

    out_renderer_backend->init = webgpu_renderer_backend_init;
    out_renderer_backend->shutdown = webgpu_renderer_backend_shutdown;
    out_renderer_backend->frame_begin = webgpu_renderer_backend_frame_begin;
    out_renderer_backend->frame_end = webgpu_renderer_backend_frame_end;
    out_renderer_backend->viewport_set = webgpu_renderer_viewport_set;
    out_renderer_backend->viewport_reset = webgpu_renderer_viewport_reset;
    out_renderer_backend->scissor_set = webgpu_renderer_scissor_set;
    out_renderer_backend->scissor_reset = webgpu_renderer_scissor_reset;
    out_renderer_backend->renderpass_begin = webgpu_renderer_renderpass_begin;
    out_renderer_backend->renderpass_end = webgpu_renderer_renderpass_end;
    out_renderer_backend->resized = webgpu_renderer_backend_on_resized;
    out_renderer_backend->geometry_draw = webgpu_renderer_geometry_draw;

    out_renderer_backend->texture_create = webgpu_renderer_texture_create;
    out_renderer_backend->texture_destroy = webgpu_renderer_texture_destroy;
    out_renderer_backend->texture_create_writeable = webgpu_renderer_texture_create_writeable;
    out_renderer_backend->texture_resize = webgpu_renderer_texture_resize;
    out_renderer_backend->texture_write_data = webgpu_renderer_texture_write_data;
    out_renderer_backend->texture_read_data = webgpu_renderer_texture_read_data;
    out_renderer_backend->texture_read_pixel = webgpu_renderer_texture_read_pixel;

    out_renderer_backend->geometry_create = webgpu_renderer_geometry_create;
    out_renderer_backend->geometry_destroy = webgpu_renderer_geometry_destroy;

    out_renderer_backend->shader_create = webgpu_renderer_shader_create;
    out_renderer_backend->shader_destroy = webgpu_renderer_shader_destroy;
    out_renderer_backend->shader_uniform_set = webgpu_renderer_uniform_set;
    out_renderer_backend->shader_init = webgpu_renderer_shader_init;
    out_renderer_backend->shader_use = webgpu_renderer_shader_use;
    out_renderer_backend->shader_bind_globals = webgpu_renderer_shader_bind_globals;
    out_renderer_backend->shader_bind_instance = webgpu_renderer_shader_bind_instance;

    out_renderer_backend->shader_apply_globals = webgpu_renderer_shader_apply_globals;
    out_renderer_backend->shader_apply_instance = webgpu_renderer_shader_apply_instance;
    out_renderer_backend->shader_instance_resources_acquire = webgpu_renderer_shader_instance_resources_acquire;
    out_renderer_backend->shader_instance_resources_release = webgpu_renderer_shader_instance_resources_release;
    out_renderer_backend->shader_after_renderpass = webgpu_shader_after_renderpass;

    out_renderer_backend->texture_map_resources_acquire = webgpu_renderer_texture_map_resources_acquire;
    out_renderer_backend->texture_map_resources_release = webgpu_renderer_texture_map_resources_release;

    out_renderer_backend->renderpass_create = webgpu_renderpass_create;
    out_renderer_backend->renderpass_destroy = webgpu_renderpass_destroy;

    out_renderer_backend->render_target_create = webgpu_renderer_render_target_create;
    out_renderer_backend->render_target_destroy = webgpu_renderer_render_target_destroy;

    out_renderer_backend->window_attachment_get = webgpu_renderer_window_attachment_get;
    out_renderer_backend->depth_attachment_get = webgpu_renderer_depth_attachment_get;
    out_renderer_backend->window_attachment_index_get = webgpu_renderer_window_attachment_index_get;
    out_renderer_backend->window_attachment_count_get = webgpu_renderer_window_attachment_count_get;

    out_renderer_backend->is_multithreaded = webgpu_renderer_is_multithreaded;
    out_renderer_backend->flag_enabled_get = webgpu_renderer_flag_enabled_get;
    out_renderer_backend->flag_enabled_set = webgpu_renderer_flag_enabled_set;

    out_renderer_backend->renderbuffer_internal_create = webgpu_buffer_create_internal;
    out_renderer_backend->renderbuffer_internal_destroy = webgpu_buffer_destroy_internal;
    out_renderer_backend->renderbuffer_bind = webgpu_buffer_bind;
    out_renderer_backend->renderbuffer_unbind = webgpu_buffer_unbind;
    out_renderer_backend->renderbuffer_map_memory = webgpu_buffer_map_memory;
    out_renderer_backend->renderbuffer_unmap_memory = webgpu_buffer_unmap_memory;
    out_renderer_backend->renderbuffer_flush = webgpu_buffer_flush;
    out_renderer_backend->renderbuffer_read = webgpu_buffer_read;
    out_renderer_backend->renderbuffer_resize = webgpu_buffer_resize;
    out_renderer_backend->renderbuffer_load_range = webgpu_buffer_load_range;
    out_renderer_backend->renderbuffer_copy_range = webgpu_buffer_copy_range;
    out_renderer_backend->renderbuffer_draw = webgpu_buffer_draw;

    return true;
}

void plugin_destroy(RENDERER_PLUGIN* renderer_backend) {
    yzero_memory(renderer_backend, sizeof(renderer_backend));
}
