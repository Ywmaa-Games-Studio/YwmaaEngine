#include "vulkan_renderer_plugin_main.h"

#include "renderer/vulkan/vulkan_backend.h"
#include <core/ymemory.h>

b8 plugin_create(RENDERER_PLUGIN* out_renderer_backend) {
    out_renderer_backend->backend_api = RENDERER_BACKEND_API_VULKAN;

    out_renderer_backend->init = vulkan_renderer_backend_init;
    out_renderer_backend->shutdown = vulkan_renderer_backend_shutdown;
    out_renderer_backend->frame_begin = vulkan_renderer_backend_frame_begin;
    out_renderer_backend->frame_end = vulkan_renderer_backend_frame_end;
    out_renderer_backend->viewport_set = vulkan_renderer_viewport_set;
    out_renderer_backend->viewport_reset = vulkan_renderer_viewport_reset;
    out_renderer_backend->scissor_set = vulkan_renderer_scissor_set;
    out_renderer_backend->scissor_reset = vulkan_renderer_scissor_reset;
    out_renderer_backend->renderpass_begin = vulkan_renderer_renderpass_begin;
    out_renderer_backend->renderpass_end = vulkan_renderer_renderpass_end;
    out_renderer_backend->resized = vulkan_renderer_backend_on_resized;
    out_renderer_backend->geometry_draw = vulkan_renderer_geometry_draw;

    out_renderer_backend->texture_create = vulkan_renderer_texture_create;
    out_renderer_backend->texture_destroy = vulkan_renderer_texture_destroy;
    out_renderer_backend->texture_create_writeable = vulkan_renderer_texture_create_writeable;
    out_renderer_backend->texture_resize = vulkan_renderer_texture_resize;
    out_renderer_backend->texture_write_data = vulkan_renderer_texture_write_data;
    out_renderer_backend->texture_read_data = vulkan_renderer_texture_read_data;
    out_renderer_backend->texture_read_pixel = vulkan_renderer_texture_read_pixel;

    out_renderer_backend->geometry_create = vulkan_renderer_geometry_create;
    out_renderer_backend->geometry_destroy = vulkan_renderer_geometry_destroy;

    out_renderer_backend->shader_create = vulkan_renderer_shader_create;
    out_renderer_backend->shader_destroy = vulkan_renderer_shader_destroy;
    out_renderer_backend->shader_uniform_set = vulkan_renderer_uniform_set;
    out_renderer_backend->shader_init = vulkan_renderer_shader_init;
    out_renderer_backend->shader_use = vulkan_renderer_shader_use;
    out_renderer_backend->shader_bind_globals = vulkan_renderer_shader_bind_globals;
    out_renderer_backend->shader_bind_instance = vulkan_renderer_shader_bind_instance;

    out_renderer_backend->shader_apply_globals = vulkan_renderer_shader_apply_globals;
    out_renderer_backend->shader_apply_instance = vulkan_renderer_shader_apply_instance;
    out_renderer_backend->shader_instance_resources_acquire = vulkan_renderer_shader_instance_resources_acquire;
    out_renderer_backend->shader_instance_resources_release = vulkan_renderer_shader_instance_resources_release;
    out_renderer_backend->shader_after_renderpass = vulkan_shader_after_renderpass;

    out_renderer_backend->texture_map_resources_acquire = vulkan_renderer_texture_map_resources_acquire;
    out_renderer_backend->texture_map_resources_release = vulkan_renderer_texture_map_resources_release;

    out_renderer_backend->render_target_create = vulkan_renderer_render_target_create;
    out_renderer_backend->render_target_destroy = vulkan_renderer_render_target_destroy;

    out_renderer_backend->renderpass_create = vulkan_renderpass_create;
    out_renderer_backend->renderpass_destroy = vulkan_renderpass_destroy;
    out_renderer_backend->window_attachment_get = vulkan_renderer_window_attachment_get;
    out_renderer_backend->depth_attachment_get = vulkan_renderer_depth_attachment_get;
    out_renderer_backend->window_attachment_index_get = vulkan_renderer_window_attachment_index_get;
    out_renderer_backend->window_attachment_count_get = vulkan_renderer_window_attachment_count_get;

    out_renderer_backend->is_multithreaded = vulkan_renderer_is_multithreaded;
    out_renderer_backend->flag_enabled_get = vulkan_renderer_flag_enabled_get;
    out_renderer_backend->flag_enabled_set = vulkan_renderer_flag_enabled_set;

    out_renderer_backend->renderbuffer_internal_create = vulkan_buffer_create_internal;
    out_renderer_backend->renderbuffer_internal_destroy = vulkan_buffer_destroy_internal;
    out_renderer_backend->renderbuffer_bind = vulkan_buffer_bind;
    out_renderer_backend->renderbuffer_unbind = vulkan_buffer_unbind;
    out_renderer_backend->renderbuffer_map_memory = vulkan_buffer_map_memory;
    out_renderer_backend->renderbuffer_unmap_memory = vulkan_buffer_unmap_memory;
    out_renderer_backend->renderbuffer_flush = vulkan_buffer_flush;
    out_renderer_backend->renderbuffer_read = vulkan_buffer_read;
    out_renderer_backend->renderbuffer_resize = vulkan_buffer_resize;
    out_renderer_backend->renderbuffer_load_range = vulkan_buffer_load_range;
    out_renderer_backend->renderbuffer_copy_range = vulkan_buffer_copy_range;
    out_renderer_backend->renderbuffer_draw = vulkan_buffer_draw;

    return true;
}

void plugin_destroy(RENDERER_PLUGIN* renderer_backend) {
    yzero_memory(renderer_backend, sizeof(renderer_backend));
}
