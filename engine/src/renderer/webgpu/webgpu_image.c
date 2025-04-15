#include "webgpu_image.h"

#include "core/ymemory.h"
#include "core/logger.h"

void webgpu_image_create(
    WEBGPU_CONTEXT* context,
    WGPUTextureDimension image_type,
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
    textureDesc.label = (WGPUStringView){"Texture Desc", sizeof("Texture Desc")};
    textureDesc.size.width = width;
    textureDesc.size.height = height;
    textureDesc.format = format;
    textureDesc.usage = usage;
    textureDesc.dimension = image_type;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.size.depthOrArrayLayers = 1;
    textureDesc.viewFormatCount = 1;
    textureDesc.viewFormats = &format;
    
    out_image->handle = wgpuDeviceCreateTexture(context->device, &textureDesc);

    // Create view
    if (create_view) {
        out_image->view = 0;
        webgpu_image_view_create(format, out_image, view_aspect_flags);
    }
}

void webgpu_image_view_create(
    WGPUTextureFormat format,
    WEBGPU_IMAGE* image,
    WGPUTextureAspect aspect_flags) {

    WGPUTextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = aspect_flags;
    textureViewDesc.label = (WGPUStringView){"Texture View Desc", sizeof("Texture View Desc")};
    // TODO: Make configurable
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D; // TODO: Make configurable.
    textureViewDesc.format = format;
    image->view = wgpuTextureCreateView(image->handle, &textureViewDesc);
}

void webgpu_image_copy_from_buffer(
    WEBGPU_CONTEXT* context,
    WEBGPU_IMAGE* image,
    const void* data,
    u8 bytes_per_pixel
    ) {

    // Arguments telling which part of the texture we upload to
    // (together with the last argument of writeTexture)
    WGPUTexelCopyTextureInfo destination;
    destination.texture = image->handle;
    destination.mipLevel = 0;
    destination.origin.x = 0; // equivalent of the offset argument of Queue::writeBuffer
    destination.origin.y = 0; // equivalent of the offset argument of Queue::writeBuffer
    destination.origin.z = 0; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All; // only relevant for depth/Stencil textures

    // Arguments telling how the C side pixel memory is laid out
    WGPUTexelCopyBufferLayout source;
    source.offset = 0;
    source.bytesPerRow = bytes_per_pixel * image->width;
    source.rowsPerImage = image->height;

    WGPUExtent3D size;
    size.width = image->width;
    size.height = image->height;
    size.depthOrArrayLayers = 1;

    const u32 required_buffer_size = image->width *image->height * bytes_per_pixel;

    wgpuQueueWriteTexture(context->queue, &destination, data, required_buffer_size, &source, &size);
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
