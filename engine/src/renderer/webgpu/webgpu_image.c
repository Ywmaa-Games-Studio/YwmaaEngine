#include "webgpu_image.h"

#include "core/ymemory.h"
#include "core/logger.h"

void webgpu_image_create(
    WEBGPU_CONTEXT* context,
    const char* name,
    E_TEXTURE_TYPE type,
    u32 width,
    u32 height,
    WGPUTextureFormat format,
    //VkImageTiling tiling,
    WGPUTextureUsage usage,
    b32 create_view,
    WGPUTextureAspect view_aspect_flags,
    WEBGPU_IMAGE* out_image) {

    // Copy params
    out_image->width = width;
    out_image->height = height;
    
    // Creation info.
    WGPUTextureDescriptor texture_desc;
    texture_desc.nextInChain = NULL;
    texture_desc.label = (WGPUStringView){name, sizeof(name)-1};
    texture_desc.size.width = width;
    texture_desc.size.height = height;
    texture_desc.format = format;
    texture_desc.usage = usage;
    switch (type) {
        default:
        case TEXTURE_TYPE_2D:
            texture_desc.dimension = WGPUTextureDimension_2D;
            texture_desc.size.depthOrArrayLayers = 1;
            break;
        case TEXTURE_TYPE_CUBE:  // Intentional, there is no cube image type.
            texture_desc.dimension = WGPUTextureDimension_2D;
            texture_desc.size.depthOrArrayLayers = 6;
            break;
    }
    
    texture_desc.mipLevelCount = 4;
    texture_desc.sampleCount = 1;
    texture_desc.viewFormatCount = 1;
    texture_desc.viewFormats = &format;
    
    out_image->handle = wgpuDeviceCreateTexture(context->device, &texture_desc);

    // Create view
    if (create_view) {
        out_image->view = 0;
        webgpu_image_view_create(name, type, format, usage, out_image, view_aspect_flags);
    }
}

void webgpu_image_view_create(
    const char* name,
    E_TEXTURE_TYPE type,
    WGPUTextureFormat format,
    WGPUTextureUsage usage,
    WEBGPU_IMAGE* image,
    WGPUTextureAspect aspect_flags) {

    WGPUTextureViewDescriptor texture_view_desc;
    texture_view_desc.aspect = aspect_flags;
    texture_view_desc.label = (WGPUStringView){name, sizeof(name)-1};
    // TODO: Make configurable
    texture_view_desc.baseArrayLayer = 0;
    texture_view_desc.baseMipLevel = 0;
    texture_view_desc.mipLevelCount = 1;
    switch (type) {
        default:
        case TEXTURE_TYPE_2D:
            texture_view_desc.arrayLayerCount = 1;
            texture_view_desc.dimension = WGPUTextureViewDimension_2D;
            break;
        case TEXTURE_TYPE_CUBE:
            texture_view_desc.arrayLayerCount = 6;
            texture_view_desc.dimension = WGPUTextureViewDimension_Cube;
            break;
    }
    texture_view_desc.format = format;
    texture_view_desc.usage = usage;
    image->view = wgpuTextureCreateView(image->handle, &texture_view_desc);
}

void webgpu_image_copy_from_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    const void* data,
    u8 bytes_per_pixel
    ) {
    // Arguments telling how the C side pixel memory is laid out
    WGPUTexelCopyBufferLayout source;
    source.offset = 0;
    source.bytesPerRow = bytes_per_pixel * image->width;
    source.rowsPerImage = image->height;

    WGPUExtent3D size;
    size.width = image->width;
    size.height = image->height;
    size.depthOrArrayLayers = 1;

    const u32 required_buffer_size = image->width * image->height * bytes_per_pixel * (type == TEXTURE_TYPE_CUBE ? 6 : 1);
    
    // Arguments telling which part of the texture we upload to
    // (together with the last argument of writeTexture)
    WGPUTexelCopyTextureInfo destination;
    destination.texture = image->handle;
    destination.mipLevel = 0;
    destination.origin.x = 0; // equivalent of the offset argument of Queue::writeBuffer
    destination.origin.y = 0; // equivalent of the offset argument of Queue::writeBuffer
    destination.origin.z = 0; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All; // only relevant for depth/Stencil textures
    switch (type)
    {
        default:
        case TEXTURE_TYPE_2D:
            wgpuQueueWriteTexture(context->queue, &destination, data, required_buffer_size, &source, &size);
            break;
        case TEXTURE_TYPE_CUBE:
            for (uint32_t layer = 0; layer < 6; ++layer) {
                source.offset = layer * (image->width * image->height * bytes_per_pixel);
                destination.origin.z = layer;
                wgpuQueueWriteTexture(context->queue, &destination, data, required_buffer_size, &source, &size);
            }
            break;
    }
    yallocate_report(required_buffer_size, MEMORY_TAG_GPU_LOCAL);
}

void webgpu_image_copy_to_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    u8 bytes_per_pixel,
    WGPUBuffer buffer,
    WGPUCommandEncoder* command_encoder) {
    
    WGPUTexelCopyTextureInfo source = (WGPUTexelCopyTextureInfo){
            .texture = image->handle,
            .mipLevel = 0,
            .origin = {0, 0, 0},
            .aspect = WGPUTextureAspect_All
    };

    WGPUTexelCopyBufferInfo destination = (WGPUTexelCopyBufferInfo){
            .buffer = buffer,
            .layout = {
                .offset = 0,
                .bytesPerRow = image->width * bytes_per_pixel,
                .rowsPerImage = image->height
            }
    };


    WGPUExtent3D size = (WGPUExtent3D){
            .width = image->width,
            .height = image->height,
            .depthOrArrayLayers = 1
    };

    switch (type)
    {
        default:
        case TEXTURE_TYPE_2D:
            wgpuCommandEncoderCopyTextureToBuffer(
                *command_encoder,
                &source,
                &destination,
                &size
            );
            break;
        case TEXTURE_TYPE_CUBE:
            for (uint32_t layer = 0; layer < 6; ++layer) {
                source.origin.z = layer;
                destination.layout.offset = layer * (image->width * image->height * bytes_per_pixel);
                wgpuCommandEncoderCopyTextureToBuffer(
                    *command_encoder,
                    &source,
                    &destination,
                    &size
                );
            }
            break;
    }

}


void webgpu_image_copy_pixel_to_buffer(
    WEBGPU_CONTEXT* context,
    E_TEXTURE_TYPE type,
    WEBGPU_IMAGE* image,
    u8 bytes_per_pixel,
    WGPUBuffer buffer,
    u32 x,
    u32 y,
    WGPUCommandEncoder* command_encoder) {
    
    WGPUTexelCopyTextureInfo source = (WGPUTexelCopyTextureInfo){
            .texture = image->handle,
            .mipLevel = 0,
            .origin = {x, y, 0},
            .aspect = WGPUTextureAspect_All
    };

    WGPUTexelCopyBufferInfo destination = (WGPUTexelCopyBufferInfo){
            .buffer = buffer,
            .layout = {
                .offset = 0,
                .bytesPerRow = 0,
                .rowsPerImage = 0
            }
    };


    WGPUExtent3D size = (WGPUExtent3D){
            .width = 1,
            .height = 1,
            .depthOrArrayLayers = 1
    };

    switch (type)
    {
        default:
        case TEXTURE_TYPE_2D:
            wgpuCommandEncoderCopyTextureToBuffer(
                *command_encoder,
                &source,
                &destination,
                &size
            );
            break;
        case TEXTURE_TYPE_CUBE:
            for (uint32_t layer = 0; layer < 6; ++layer) {
                source.origin.z = layer;
                destination.layout.offset = layer * (image->width * image->height * bytes_per_pixel);
                wgpuCommandEncoderCopyTextureToBuffer(
                    *command_encoder,
                    &source,
                    &destination,
                    &size
                );
            }
            break;
    }
}

void webgpu_image_destroy(WEBGPU_IMAGE* image, u8 channel_count, E_TEXTURE_TYPE type) {
    if (image->view) {
        wgpuTextureViewRelease(image->view);
        image->view = 0;
    }
    if (image->handle) {
        const u32 buffer_size = image->width * image->height * channel_count * (type == TEXTURE_TYPE_CUBE ? 6 : 1);
        yfree_report(buffer_size, MEMORY_TAG_GPU_LOCAL);
        wgpuTextureDestroy(image->handle);
        wgpuTextureRelease(image->handle);
        image->handle = 0;
    }
}
