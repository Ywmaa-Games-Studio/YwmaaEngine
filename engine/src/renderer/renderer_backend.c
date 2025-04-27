#include "renderer_backend.h"

#include "vulkan/vulkan_backend.h"
#include "webgpu/webgpu_backend.h"

b8 renderer_backend_create(E_RENDERER_BACKEND_API type, RENDERER_BACKEND* out_renderer_backend) {

    if (type == RENDERER_BACKEND_API_VULKAN) {
        out_renderer_backend->init = vulkan_renderer_backend_init;
        out_renderer_backend->shutdown = vulkan_renderer_backend_shutdown;
        out_renderer_backend->begin_frame = vulkan_renderer_backend_begin_frame;
        out_renderer_backend->update_global_state = vulkan_renderer_update_global_state;
        out_renderer_backend->end_frame = vulkan_renderer_backend_end_frame;
        out_renderer_backend->resized = vulkan_renderer_backend_on_resized;
        out_renderer_backend->draw_geometry = vulkan_renderer_draw_geometry;
        out_renderer_backend->create_texture = vulkan_renderer_create_texture;
        out_renderer_backend->destroy_texture = vulkan_renderer_destroy_texture;
        out_renderer_backend->create_material = vulkan_renderer_create_material;
        out_renderer_backend->destroy_material = vulkan_renderer_destroy_material;
        out_renderer_backend->create_geometry = vulkan_renderer_create_geometry;
        out_renderer_backend->destroy_geometry = vulkan_renderer_destroy_geometry;
        return true;
    }

    if (type == RENDERER_BACKEND_API_WEBGPU) {
        out_renderer_backend->init = webgpu_renderer_backend_init;
        out_renderer_backend->shutdown = webgpu_renderer_backend_shutdown;
        out_renderer_backend->begin_frame = webgpu_renderer_backend_begin_frame;
        out_renderer_backend->update_global_state = webgpu_renderer_update_global_state;
        out_renderer_backend->end_frame = webgpu_renderer_backend_end_frame;
        out_renderer_backend->resized = webgpu_renderer_backend_on_resized;
        out_renderer_backend->draw_geometry = webgpu_renderer_draw_geometry;
        out_renderer_backend->create_texture = webgpu_renderer_create_texture;
        out_renderer_backend->destroy_texture = webgpu_renderer_destroy_texture;
        out_renderer_backend->create_material = webgpu_renderer_create_material;
        out_renderer_backend->destroy_material = webgpu_renderer_destroy_material;
        out_renderer_backend->create_geometry = webgpu_renderer_create_geometry;
        out_renderer_backend->destroy_geometry = webgpu_renderer_destroy_geometry;
        return true;
    }

    return false;
}

void renderer_backend_destroy(RENDERER_BACKEND* renderer_backend) {
    renderer_backend->init = 0;
    renderer_backend->shutdown = 0;
    renderer_backend->begin_frame = 0;
    renderer_backend->update_global_state = 0;
    renderer_backend->end_frame = 0;
    renderer_backend->resized = 0;
    renderer_backend->draw_geometry = 0;
    renderer_backend->create_texture = 0;
    renderer_backend->destroy_texture = 0;
    renderer_backend->create_material = 0;
    renderer_backend->destroy_material = 0;
    renderer_backend->create_geometry = 0;
    renderer_backend->destroy_geometry = 0;
}
