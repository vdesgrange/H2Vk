#include "vk_shaders.h"
#include "vk_device.h"
#include "utilities/vk_helpers.h"

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

bool Shader::load_shader_module(const Device& device, const char* filePath, VkShaderModule* out) {
    const std::vector<uint32_t> code = Helper::read_file(filePath);
    return create_shader_module(device, code, out);
}

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