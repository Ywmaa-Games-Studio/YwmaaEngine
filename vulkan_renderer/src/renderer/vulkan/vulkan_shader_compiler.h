// compiler glsl to spirv
#include "defines.h"
#include "../thirdparty/glslang/glslang/Include/glslang_c_interface.h"
// Required for use of glslang_default_resource
#include "../thirdparty/glslang/glslang/Public/resource_limits_c.h"
typedef struct SpirVBinary {
    u32 *words; // SPIR-V words
    int size; // number of words in SPIR-V binary
} SpirVBinary;

SpirVBinary compileShaderToSPIRV_Vulkan(glslang_stage_t stage, const char* shaderSource, const char* fileName);
void spirv_binary_free(SpirVBinary* bin);
