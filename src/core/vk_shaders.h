#pragma once

#include "vulkan/vulkan_core.h"

#include <vector>

struct ShaderEffect {
    VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> setLayouts;

    struct ShaderStage {
        VkShaderModule* shaderModule;
        VkShaderStageFlagBits flags;
    };

    std::vector<ShaderStage> shaderStages;
};

struct ShaderPass {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    ShaderEffect effect;
};


