#pragma once

#include <vector>

#include "vk_shaders.h"
#include "core/manager/vk_system_manager.h"

class Device;
class RenderPass;

class PipelineBuilder {
public:
    PipelineBuilder(const Device& device): _device(device) {};

    std::shared_ptr<ShaderEffect> build_effect(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants, std::vector<std::pair<VkShaderStageFlagBits, const char*>> shaderModules);
    std::shared_ptr<ShaderPass> build_pass(std::shared_ptr<ShaderEffect> effect);

protected:
    const class Device& _device;

private:
    VkPipelineLayout build_layout(std::vector<VkDescriptorSetLayout> &setLayouts, std::vector<VkPushConstantRange> &pushConstants) const;
    virtual VkPipeline build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) { return {}; };
};

class GraphicPipeline final : public PipelineBuilder {
public:
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo _colorBlending;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    std::vector<VkDynamicState> _dynamicStateEnables;

    GraphicPipeline(const Device& device, RenderPass& renderPass);

private:
    const class RenderPass& _renderPass;

    VkPipeline build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) override;
};

class ComputePipeline final : public PipelineBuilder {
public:
    ComputePipeline(const Device& device) : PipelineBuilder(device) {};

private:
    VkPipeline build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) override;
};