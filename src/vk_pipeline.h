#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "vk_types.h"
#include "vk_material.h"

class Window;
class Device;
class RenderPass;

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;

    VkPipelineLayout _triPipelineLayout;
    VkPipelineLayout _meshPipelineLayout;

    VkPipeline _graphicsPipeline;
    VkPipeline _redTrianglePipeline;
    VkPipeline _meshPipeline;

    std::unordered_map<std::string, Material> _materials;

    PipelineBuilder(const Window& window, const Device& device, RenderPass& renderPass, VkDescriptorSetLayout& globalSetLayout);
    ~PipelineBuilder();

    bool load_shader_module(const char* filePath, VkShaderModule* out);

    bool create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out);

    VkPipeline build_pipeline(const Device& device, RenderPass& renderPass);

    Material* create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string &name);

    Material* get_material(const std::string &name);

private:
    const class Device& _device;

    VkPipelineLayout* _pipelineLayout;

};