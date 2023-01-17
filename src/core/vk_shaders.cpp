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