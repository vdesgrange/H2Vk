#pragma once

#include "vulkan/vulkan_core.h"

#include <vector>

struct ShaderEffect {
    struct ShaderStage { // Temporary structure used by VkPipelineShaderStageCreateInfo
        VkShaderModule shaderModule;
        VkShaderStageFlagBits flags;
    };

    VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<ShaderStage> shaderStages;
};

struct ShaderPass {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    ShaderEffect effect;
};


