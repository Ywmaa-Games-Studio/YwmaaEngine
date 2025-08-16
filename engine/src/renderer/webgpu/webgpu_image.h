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
 * @param context The WebGPU context.
 * @param image The image to copy the buffer's data to.
 */
void webgpu_image_copy_from_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    const void* data,
    u8 bytes_per_pixel
    );

/**
 * @brief Copies data in the provided image to the given buffer.
 *
 * @param context The WebGPU context.
 * @param type The type of texture. Provides hints to layer count.
 * @param image The image to copy the image's data from.
 * @param buffer The buffer to copy to.
 * @param command_buffer The command buffer to be used for the copy.
 */
void webgpu_image_copy_to_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    u8 bytes_per_pixel,
    WGPUBuffer buffer,
    WGPUCommandEncoder* command_encoder);

/**
 * @brief Copies a single pixel's data from the given image to the provided buffer.
 * 
 * @param context The WebGPU context.
 * @param type The type of texture. Provides hints to layer count.
 * @param image The image to copy the image's data from.
 * @param buffer The buffer to copy to.
 * @param x The x-coordinate of the pixel to copy.
 * @param y The y-coordinate of the pixel to copy.
 * @param command_buffer The command buffer to be used for the copy.
 */
void webgpu_image_copy_pixel_to_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    u8 bytes_per_pixel,
    WGPUBuffer buffer,
    u32 x,
    u32 y,
    WGPUCommandEncoder* command_encoder);


void webgpu_image_destroy(WEBGPU_IMAGE* image, u8 channel_count, E_TEXTURE_TYPE type);

