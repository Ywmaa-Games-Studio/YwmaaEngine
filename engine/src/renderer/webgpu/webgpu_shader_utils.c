#include "webgpu_shader_utils.h"

#include "core/ystring.h"
#include "core/logger.h"
#include "core/ymemory.h"

#include "systems/resource_system.h"

b8 webgpu_create_shader_module(WEBGPU_CONTEXT* context, WGPUShaderModule* shader_module, const char* name) {

    WGPUShaderModuleDescriptor shaderDesc = {0};

    WGPUShaderSourceWGSL shaderCodeDesc = {0};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = NULL;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    // Build file name.
    char file_name[512];
    string_format(file_name, "%s", name);

    // Read the resource.
    RESOURCE text_resource;
    if (!resource_system_load(file_name, RESOURCE_TYPE_TEXT, &text_resource)) {
        PRINT_ERROR("Unable to read shader module: %s.", file_name);
        return false;
    }

    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.code = (WGPUStringView){(char*)text_resource.data, text_resource.data_size};
    (*shader_module) = wgpuDeviceCreateShaderModule(context->device, &shaderDesc);

    // Release the resource.
    resource_system_unload(&text_resource);

    return true;
}
