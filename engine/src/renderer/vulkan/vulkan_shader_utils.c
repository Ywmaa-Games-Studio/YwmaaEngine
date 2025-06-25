#include "vulkan_shader_utils.h"

#include "core/ystring.h"
#include "core/logger.h"
#include "core/ymemory.h"

#include "systems/resource_system.h"
#include "shader_compiler.h"

b8 create_shader_module(
    VULKAN_CONTEXT* context,
    const char* name,
    const char* type_str,
    VkShaderStageFlagBits shader_stage_flag,
    u32 stage_index,
    VULKAN_SHADER_STAGE* shader_stages) {
    // Build file name, which will also be used as the resource name..
    char file_name[512];
    string_format(file_name, "shaders/%s.%s.glsl", name, type_str);

    // Read the resource.
    RESOURCE text_resource;
    if (!resource_system_load(file_name, RESOURCE_TYPE_TEXT, &text_resource)) {
        PRINT_ERROR("Unable to read shader module: %s.", file_name);
        return false;
    }
    const u8 stage_flag_to_glslang_stage[] = {
        [VK_SHADER_STAGE_VERTEX_BIT] = GLSLANG_STAGE_VERTEX,
        [VK_SHADER_STAGE_FRAGMENT_BIT] = GLSLANG_STAGE_FRAGMENT,
        [VK_SHADER_STAGE_COMPUTE_BIT] = GLSLANG_STAGE_COMPUTE,
        [VK_SHADER_STAGE_GEOMETRY_BIT] = GLSLANG_STAGE_GEOMETRY,
    };
    // Compile the shader to SPIR-V binary.
    SpirVBinary binary_resource = compileShaderToSPIRV_Vulkan(
        stage_flag_to_glslang_stage[shader_stage_flag],
        (const char*)text_resource.data,
        file_name);
    
    yzero_memory(&shader_stages[stage_index].create_info, sizeof(VkShaderModuleCreateInfo));
    shader_stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // Use the resource's size and data directly.
    shader_stages[stage_index].create_info.codeSize = binary_resource.size;
    shader_stages[stage_index].create_info.pCode = (u32*)binary_resource.words;

    VK_CHECK(vkCreateShaderModule(
        context->device.logical_device,
        &shader_stages[stage_index].create_info,
        context->allocator,
        &shader_stages[stage_index].handle));

   // Release the resource.
   resource_system_unload(&text_resource);

    // Shader stage info
    yzero_memory(&shader_stages[stage_index].shader_stage_create_info, sizeof(VkPipelineShaderStageCreateInfo));
    shader_stages[stage_index].shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[stage_index].shader_stage_create_info.stage = shader_stage_flag;
    shader_stages[stage_index].shader_stage_create_info.module = shader_stages[stage_index].handle;
    shader_stages[stage_index].shader_stage_create_info.pName = "main";

    return true;
}
