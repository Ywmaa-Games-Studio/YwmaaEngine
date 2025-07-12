#pragma once

#include "webgpu_types.inl"

void webgpu_image_create(
    WEBGPU_CONTEXT* context,
    const char* name,
    E_TEXTURE_TYPE type,
    u32 width,
    u32 height,
    WGPUTextureFormat format,
    //WGPUImageTiling tiling,
    WGPUTextureUsage usage,
    b32 create_view,
    WGPUTextureAspect view_aspect_flags,
    WEBGPU_IMAGE* out_image);

void webgpu_image_view_create(
    const char* name,
    E_TEXTURE_TYPE type,
    WGPUTextureFormat format,
    WGPUTextureUsage usage,
    WEBGPU_IMAGE* image,
    WGPUTextureAspect aspect_flags);

/**
 * Copies data in buffer to provided image.
 * @param context The Vulkan context.
 * @param image The image to copy the buffer's data to.
 */
void webgpu_image_copy_from_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    const void* data,
    u8 bytes_per_pixel
    );


void webgpu_image_destroy(WEBGPU_IMAGE* image);

