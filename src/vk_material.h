#pragma once

#include "vk_types.h"

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    Material() = default;
    Material(VkPipeline pipeline, VkPipelineLayout pipelineLayout) : pipeline(pipeline), pipelineLayout(pipelineLayout) {};
};