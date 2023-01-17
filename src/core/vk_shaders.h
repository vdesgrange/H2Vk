#pragma once

#include "vulkan/vulkan_core.h"

#include <vector>

class Device;

struct ShaderEffect {
    struct ShaderStage { // Temporary structure used by VkPipelineShaderStageCreateInfo
        VkShaderStageFlagBits flags;
        VkShaderModule shaderModule;
    };

    // VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstants;
    std::vector<ShaderStage> shaderStages;
};

struct ShaderPass {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::shared_ptr<ShaderEffect> effect;
};

typedef ShaderPass Material;

class Shader final {
public:
    static bool load_shader_module(const Device& device, const char* filePath, VkShaderModule* out);
    static bool create_shader_module(const Device& device, const std::vector<uint32_t>& code, VkShaderModule* out);
};