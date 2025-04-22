#include "webgpu_shader_utils.h"

#include "core/ystring.h"
#include "core/logger.h"
#include "core/ymemory.h"

#include "io/filesystem.h"

b8 webgpu_create_shader_module(WEBGPU_CONTEXT* context, WGPUShaderModule* shader_module) {

    WGPUShaderModuleDescriptor shaderDesc = {};

    WGPUShaderSourceWGSL shaderCodeDesc = {};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = NULL;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    // Build file name.
    char file_name[512] = "assets/shaders/shader.wgsl";

    // Obtain file handle.
    FILE_HANDLE handle;
    if (!filesystem_open(file_name, FILE_MODE_READ, false, &handle)) {
        PRINT_ERROR("Unable to read wgsl shader: %s.", file_name);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&handle, &file_size)) {
        PRINT_ERROR("Unable to wgsl shader file: %s.", file_name);
        filesystem_close(&handle);
        return false;
    }

    // Read the entire file as binary.
    char* code = yallocate_aligned(sizeof(char) * file_size+1, 4, MEMORY_TAG_ARRAY);
    u64 read_size = 0;
    if (!filesystem_read_all_text(&handle, code, &read_size)) {
        PRINT_ERROR("Unable to text read shader module %s.", file_name);
        filesystem_close(&handle);
        return false;
    }

    // Close the file.
    filesystem_close(&handle);

    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.code = (WGPUStringView){code, read_size};
    (*shader_module) = wgpuDeviceCreateShaderModule(context->device, &shaderDesc);

    yfree_aligned(code, sizeof(char) * file_size+1, 4, MEMORY_TAG_ARRAY);

    return true;
}