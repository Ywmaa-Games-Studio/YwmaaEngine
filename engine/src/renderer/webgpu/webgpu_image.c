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
    WGPUTextureDescriptor textureDesc;
    textureDesc.nextInChain = NULL;
    textureDesc.label = (WGPUStringView){name, sizeof(name)-1};
    textureDesc.size.width = width;
    textureDesc.size.height = height;
    textureDesc.format = format;
    textureDesc.usage = usage;
    switch (type) {
        default:
        case TEXTURE_TYPE_2D:
            textureDesc.dimension = WGPUTextureDimension_2D;
            textureDesc.size.depthOrArrayLayers = 1;
            break;
        case TEXTURE_TYPE_CUBE:  // Intentional, there is no cube image type.
            textureDesc.dimension = WGPUTextureDimension_2D;
            textureDesc.size.depthOrArrayLayers = 6;
            break;
    }
    
    textureDesc.mipLevelCount = 4;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 1;
    textureDesc.viewFormats = &format;
    
    out_image->handle = wgpuDeviceCreateTexture(context->device, &textureDesc);

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

    WGPUTextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = aspect_flags;
    textureViewDesc.label = (WGPUStringView){name, sizeof(name)-1};
    // TODO: Make configurable
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    switch (type) {
        default:
        case TEXTURE_TYPE_2D:
            textureViewDesc.arrayLayerCount = 1;
            textureViewDesc.dimension = WGPUTextureViewDimension_2D;
            break;
        case TEXTURE_TYPE_CUBE:
            textureViewDesc.arrayLayerCount = 6;
            textureViewDesc.dimension = WGPUTextureViewDimension_Cube;
            break;
    }
    textureViewDesc.format = format;
    textureViewDesc.usage = usage;
    image->view = wgpuTextureCreateView(image->handle, &textureViewDesc);
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
}

void webgpu_image_destroy(WEBGPU_IMAGE* image) {
    if (image->view) {
        wgpuTextureViewRelease(image->view);
        image->view = 0;
    }
    if (image->handle) {
        wgpuTextureDestroy(image->handle);
        wgpuTextureRelease(image->handle);
        image->handle = 0;
    }
}
