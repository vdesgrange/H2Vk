/*
*  H2Vk - Shader class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_shaders.h"
#include "core/utilities/vk_resources.h"

/**
 * Create a shader module.
 * Shader modules contain shader code and one or more entry points.
 * Shaders are selected from a shader module by specifying an entry point as part of pipeline creation.
 *
 * @brief create shader module object
 * @param device vulkan device wrapper
 * @param code shader code content
 * @param out pointer to shader module object being created
 * @return true if shader module creation is successful
 * @return false if shader module creation fail
 */
bool Shader::create_shader_module(const Device& device, const std::vector<uint32_t>& code, VkShaderModule* out) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device._logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    *out = shaderModule;
    return true;
}

/**
 *  Read shader file then create a shader module.
 *  Shader modules contain shader code from the external file.
 * @brief load a shader file and create shader module
 * @param device vulkan device wrapper
 * @param filePath path to file containing shader code
 * @param out pointer to shader module object being created
 * @return true if shader module creation is successful
 * @return false if shader module creation fail
 */
bool Shader::load_shader_module(const Device& device, const char* filePath, VkShaderModule* out) {
    const std::vector<uint32_t> code = helper::read_file(filePath);
    return create_shader_module(device, code, out);
}

/**
 * Map ShaderType enumeration to a vulkan shader stage bit
 * @param type shader type
 * @return vulkan shader stage bit
 */
VkShaderStageFlagBits Shader::get_shader_stage(ShaderType type) {
    switch (type) {
        case ShaderType::VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderType::TESSELATION_CONTROL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderType::TESSELATION_EVALUATION:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderType::GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderType::COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            throw std::invalid_argument("Invalid shader type");
    }
}