#pragma once

#include "defines.h"
#include <webgpu/webgpu.h>




typedef struct WEBGPU_CONTEXT {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUSurface surface;
    WGPUQueue queue;

} WEBGPU_CONTEXT;