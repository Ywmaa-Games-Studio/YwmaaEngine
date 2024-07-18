#pragma once

#include "defines.h"

struct PLATFORM_STATE;
struct WEBGPU_CONTEXT;

b8 platform_create_webgpu_surface(
    struct PLATFORM_STATE* platform_state,
    struct WEBGPU_CONTEXT* context);