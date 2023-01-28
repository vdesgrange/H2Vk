#pragma once

#include "vulkan/vulkan_core.h"

#include <vector>
#include <iostream>
#include <memory>

class Device;

struct ShaderEffect {
    struct ShaderStage { // Temporary structure used by VkPipelineShaderStageCreateInfo
        VkShaderStageFlagBits flags;
        VkShaderModule shaderModule;
    };

    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstants;
    std::vector<ShaderStage> shaderStages;

    ~ShaderEffect() {
        std::cout << "ShaderEffect destroyed" << std::endl;
    }
};

struct ShaderPass {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::shared_ptr<ShaderEffect> effect;

    ~ShaderPass() {
        std::cout << "Material/ShaderPass destroyed" << std::endl;  // todo: move _materials destruction of pipeline here?
    }
};

typedef ShaderPass Material;

class Shader final {
public:
    static bool load_shader_module(const Device& device, const char* filePath, VkShaderModule* out);
    static bool create_shader_module(const Device& device, const std::vector<uint32_t>& code, VkShaderModule* out);
};