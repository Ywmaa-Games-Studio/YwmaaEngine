#include "platform/platform.h"

#if defined(YPLATFORM_APPLE)

#include <data_structures/darray.h>
#include <platform/platform.h>
#include <core/ymemory.h>
#include <core/logger.h>

#ifdef __OBJC__
@class CAMetalLayer;
#else
typedef void CAMetalLayer;
#endif

typedef struct MACOS_HANDLE_INFO {
    CAMetalLayer* layer;
} MACOS_HANDLE_INFO;

#include "renderer/webgpu/webgpu_types.inl"

// Surface creation for WebGPU
b8 platform_create_webgpu_surface(WEBGPU_CONTEXT *context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    WGPUSurfaceSourceMetalLayer surface_metal_layer;
    surface_metal_layer.chain.next = NULL;
    surface_metal_layer.layer = ((MACOS_HANDLE_INFO*)block)->layer;

    WGPUSurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &surface_metal_layer.chain;
    surfaceDescriptor.label = (WGPUStringView){"MacOS Surface", sizeof("MacOS Surface")};

    context->surface = wgpuInstanceCreateSurface(context->instance, &surfaceDescriptor);

    return true;
}

#endif
