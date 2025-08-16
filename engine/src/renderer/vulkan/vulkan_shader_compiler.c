// compiler glsl to spirv
#include "vulkan_shader_compiler.h"
#include "core/logger.h"
#include "core/ymemory.h"

SpirVBinary compileShaderToSPIRV_Vulkan(glslang_stage_t stage, const char* shaderSource, const char* fileName) {
    PRINT_DEBUG("Compiling shader '%s' stage %i to SPIR-V...", fileName, stage);
    
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_4,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = shaderSource,
        .default_version = 450,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = true,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    SpirVBinary bin = {
        .words = NULL,
        .size = 0,
    };
    if (!glslang_shader_preprocess(shader, &input))	{
        PRINT_ERROR("GLSL preprocessing failed %s\n", fileName);
        PRINT_ERROR("%s\n", glslang_shader_get_info_log(shader));
        PRINT_ERROR("%s\n", glslang_shader_get_info_debug_log(shader));
        PRINT_ERROR("%s\n", input.code);
        glslang_shader_delete(shader);
        return bin;
    }

    if (!glslang_shader_parse(shader, &input)) {
        PRINT_ERROR("GLSL parsing failed %s\n", fileName);
        PRINT_ERROR("%s\n", glslang_shader_get_info_log(shader));
        PRINT_ERROR("%s\n", glslang_shader_get_info_debug_log(shader));
        PRINT_ERROR("%s\n", glslang_shader_get_preprocessed_code(shader));
        glslang_shader_delete(shader);
        return bin;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        PRINT_ERROR("GLSL linking failed %s\n", fileName);
        PRINT_ERROR("%s\n", glslang_program_get_info_log(program));
        PRINT_ERROR("%s\n", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return bin;
    }

    glslang_program_SPIRV_generate(program, stage);

    bin.size = glslang_program_SPIRV_get_size(program) * sizeof(u32);
    bin.words = yallocate(bin.size, MEMORY_TAG_ARRAY);
    glslang_program_SPIRV_get(program, bin.words);

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
        PRINT_DEBUG("(%s) %s\b", fileName, spirv_messages);

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    PRINT_DEBUG("Shader '%s' compiled successfully to SPIR-V with %d words.", fileName, bin.size / sizeof(u32));
    return bin;
}

void spirv_binary_free(SpirVBinary* bin) {
    if (bin && bin->words) {
        yfree(bin->words);
        bin->words = NULL;
        bin->size = 0;
    }
}
