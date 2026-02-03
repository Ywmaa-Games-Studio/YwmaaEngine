#include "platform/platform.h"
// Windows Platform Layer.
#if YPLATFORM_WEB

#include <platform/platform.h>
#include <core/ymemory.h>
#include <core/logger.h>

const char* web_canvas_name_hash = "#RenderingSurface";

#include "renderer/webgpu/webgpu_types.inl"
   
// Surface creation for WebGPU
b8 platform_create_webgpu_surface(struct WEBGPU_CONTEXT* context) {
    if (!context) {
        return false;
    }

    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc = {
        .chain = {
            .sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector,
        },
        .selector = web_canvas_name_hash
    };
    
    WGPUSurfaceDescriptor surface_desc = {0};
    surface_desc.nextInChain = (WGPUChainedStruct*)&canvas_desc;
    //surface_desc.label = NULL;
    
    context->surface = wgpuInstanceCreateSurface(context->instance, &surface_desc);

    return true;
}
#endif
