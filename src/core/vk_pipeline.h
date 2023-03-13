#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "core/utilities/vk_types.h"
#include "core/manager/vk_system_manager.h"
#include "vk_shaders.h"

class Window;
class Device;
class RenderPass;

class PipelineBuilder {
public:
    enum Type { graphic, compute };

    Type _type = Type::graphic;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;

    PipelineBuilder(const Device& device, RenderPass& renderPass);

    std::shared_ptr<ShaderEffect> build_effect(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants, std::vector<std::pair<VkShaderStageFlagBits, const char*>> shaderModules);
    std::shared_ptr<ShaderEffect> build_effect(std::vector<VkDescriptorSetLayout> setLayouts,  std::vector<VkPushConstantRange> pushConstants, std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> shaderModules);
    std::shared_ptr<ShaderPass> build_pass(std::shared_ptr<ShaderEffect> effect);

private:
    const class Device& _device;
    const class RenderPass& _renderPass;

    VkPipelineLayout build_layout(std::vector<VkDescriptorSetLayout> &setLayouts, std::vector<VkPushConstantRange> &pushConstants) const;
    VkPipeline build_pipeline(const Device& device, const RenderPass& renderPass, VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);

};
