#include "platform/platform.h"
// Windows Platform Layer.
#if YPLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <data_structures/darray.h>
#include <platform/platform.h>
#include <core/ymemory.h>
#include <core/logger.h>

typedef struct WIN32_HANDLE_INFO {
    HINSTANCE h_instance;
    HWND hwnd;
} WIN32_HANDLE_INFO;

#include "renderer/webgpu/webgpu_types.inl"

   
// Surface creation for WebGPU
b8 platform_create_webgpu_surface(WEBGPU_CONTEXT *context) {
    u64 size = 0;
    platform_get_handle_info(&size, 0);
    void *block = yallocate_aligned(size, 8, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, block);

    WIN32_HANDLE_INFO *handle = (WIN32_HANDLE_INFO *)block;

    if (!handle) {
        return false;
    }

    WGPUSurfaceSourceWindowsHWND fromWindowsHWND;
    fromWindowsHWND.chain.next = NULL;
    fromWindowsHWND.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    fromWindowsHWND.hinstance = handle->h_instance;
    fromWindowsHWND.hwnd = handle->hwnd;

    WGPUSurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &fromWindowsHWND.chain;
    surfaceDescriptor.label = (WGPUStringView){"Windows Surface", sizeof("Windows Surface")};

    context->surface = wgpuInstanceCreateSurface(context->instance, &surfaceDescriptor);

    return true;
}
#endif
