#pragma once

#include "defines.h"
#include <webgpu/webgpu.h>




typedef struct WEBGPU_CONTEXT {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;

} WEBGPU_CONTEXT;