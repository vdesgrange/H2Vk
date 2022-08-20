#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

#include "vk_types.h"

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineLayout _pipelineLayout;

    VkPipeline build_pipeline(VkDevice device, VkRenderPass renderPass);
};