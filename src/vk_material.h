#pragma once

#include "vk_types.h"

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet textureSet{VK_NULL_HANDLE};

    Material() = default;
    Material(VkPipeline pipeline, VkPipelineLayout pipelineLayout) : pipeline(pipeline), pipelineLayout(pipelineLayout) {};
};