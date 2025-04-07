#include "webgpu_shader_utils.h"

#include "core/ystring.h"
#include "core/logger.h"
#include "core/ymemory.h"

#include "io/filesystem.h"

void webgpu_create_bind_group(WEBGPU_CONTEXT* context);
void bind_layout_set_default(WGPUBindGroupLayoutEntry *bindingLayout);

b8 webgpu_create_shader_module(WEBGPU_CONTEXT* context, WGPUShaderModule* shader_module) {

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.hintCount = 0;
    shaderDesc.hints = NULL;

    WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = NULL;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
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
    u64 size = 0;
    u8 extra_size = 1; // This is required to add extra size for empty space
    char* code = yallocate(sizeof(char) * file_size +extra_size, MEMORY_TAG_STRING);
    if (!filesystem_read_all_text(&handle, code, &size)) {
        PRINT_ERROR("Unable to text read shader module: %s.", file_name);
        return false;
    }

    // Close the file.
    filesystem_close(&handle);

    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.code = code;
    (*shader_module) = wgpuDeviceCreateShaderModule(context->device, &shaderDesc);

    return true;
}