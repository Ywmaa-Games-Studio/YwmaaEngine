#include "vulkan_object_shader.h"

#include "core/logger.h"

#include "renderer/vulkan/vulkan_shader_utils.h"

#define BUILTIN_SHADER_NAME_OBJECT "builtin.shader"


b8 vulkan_object_shader_create(VULKAN_CONTEXT* context, VULKAN_OBJECT_SHADER* out_shader) {
    // Shader module init per stage.
    char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {
            PRINT_ERROR("Unable to create %s shader module for '%s'.", stage_type_strs[i], BUILTIN_SHADER_NAME_OBJECT);
            return false;
        }
    }

    // Descriptors

    return true;
}

void vulkan_object_shader_destroy(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader) {

}

void vulkan_object_shader_use(VULKAN_CONTEXT* context, struct VULKAN_OBJECT_SHADER* shader) {

}