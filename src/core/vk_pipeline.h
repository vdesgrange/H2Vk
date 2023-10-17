/*
*  H2Vk - Pipeline class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vector>
#include <unordered_map>

#include "vk_shaders.h"
#include "components/model/vk_model.h"
#include "core/manager/vk_system_manager.h"

class Device;
class RenderPass;
class VertexInputDescription;

/**
 * @brief Tools common to pipeline
 */
class PipelineBuilder {
public:
    explicit PipelineBuilder(const Device& device): _device(device) {};

    std::shared_ptr<ShaderEffect> build_effect(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants, std::vector<std::tuple<VkShaderStageFlagBits, const char*, VkSpecializationInfo>> shaderModules);
    std::shared_ptr<ShaderPass> build_pass(std::shared_ptr<ShaderEffect> effect);

protected:
    /** @brief vulkan device wrapper */
    const class Device& _device;

private:
    VkPipelineLayout build_layout(std::vector<VkDescriptorSetLayout> &setLayouts, std::vector<VkPushConstantRange> &pushConstants) const;
    virtual VkPipeline build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) { return {}; };
};

/**
 * @brief Graphics pipeline wrapper
 */
class GraphicPipeline final : public PipelineBuilder {
public:
    /** @brief configuration of drawn topology (ie. point, line, triangle) */
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    /** @brief configuration used in rasterization stage (ie. polygon mode, depth bias, etc.) */
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    /** @brief controls how pipeline blends into a given color attachment */
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    /** @brief contains the information about the color attachments and how they are used */
    VkPipelineColorBlendStateCreateInfo _colorBlending;
    /** @brief configuration for MSAA */
    VkPipelineMultisampleStateCreateInfo _multisampling;
    /** @brief configuration about how to use depth-testing (ie. depthTestEnable, depthCompareOp) */
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    /** @brief information for vertex buffers and formats */
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    /** @brief collection of dynamic states (viewport, scissor, etc.) */
    std::vector<VkDynamicState> _dynamicStateEnables;
    /** @brief vertex binding and attributes used by VkPipelineVertexInputStateCreateInfo */
    VertexInputDescription _vertexDescription;

    GraphicPipeline(const Device& device, RenderPass& renderPass);

private:
    /** @brief render pass wrapper object describing the environment in which the pipeline will be used */
    const class RenderPass& _renderPass;

    VkPipeline build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) override;
};

/**
 * @brief Compute pipeline wrapper
 */
class ComputePipeline final : public PipelineBuilder {
public:
    explicit ComputePipeline(const Device& device) : PipelineBuilder(device) {};

private:
    VkPipeline build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) override;
};