
#include <iostream>

#include "vk_pipeline.h"
#include "vk_device.h"
#include "vk_renderpass.h"
#include "core/model/vk_model.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"

std::shared_ptr<ShaderEffect> PipelineBuilder::build_effect(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants, std::vector<std::pair<VkShaderStageFlagBits, const char*>> shaderModules) {
    std::shared_ptr<ShaderEffect> effect = std::make_shared<ShaderEffect>();
    effect->setLayouts = setLayouts;
    effect->pushConstants = pushConstants;

    for (auto const& module: shaderModules) {
        VkShaderModule shader;
        if (!Shader::load_shader_module(_device, module.second, &shader)) {
            std::cout << "Error when building the shader module" << std::string(module.second) << std::endl;
        }
        effect->shaderStages.push_back(ShaderEffect::ShaderStage{module.first, shader});
    }

    return effect;
}

std::shared_ptr<ShaderPass> PipelineBuilder::build_pass(std::shared_ptr<ShaderEffect> effect) {
    std::shared_ptr<ShaderPass> pass = std::make_shared<ShaderPass>();
    pass->effect = std::shared_ptr<ShaderEffect>(effect);
    pass->pipelineLayout = this->build_layout(effect->setLayouts, effect->pushConstants);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
    for (const auto& stage : pass->effect->shaderStages) {
        shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(stage.flags, stage.shaderModule));
    }

    pass->pipeline =  this->build_pipeline(pass->pipelineLayout, shaderStages);

    return pass;
}

VkPipelineLayout PipelineBuilder::build_layout(std::vector<VkDescriptorSetLayout> &setLayouts, std::vector<VkPushConstantRange> &pushConstants) const {
    VkPipelineLayout layout{};
    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    layoutInfo.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();

    VK_CHECK(vkCreatePipelineLayout(_device._logicalDevice, &layoutInfo, nullptr, &layout));

    return layout;
}

GraphicPipeline::GraphicPipeline(const Device& device, RenderPass& renderPass) : PipelineBuilder(device), _renderPass(renderPass) {
    _inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _depthStencil = vkinit::pipeline_depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    _rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    _multisampling = vkinit::multisampling_state_create_info();
    _colorBlendAttachment = vkinit::color_blend_attachment_state();
    _colorBlending = vkinit::color_blend_state_create_info(&_colorBlendAttachment);
    _dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    _vertexInputInfo = vkinit::vertex_input_state_create_info();
    // VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::vertex_input_state_create_info();
    _vertexDescription = Vertex::get_vertex_description();
    _vertexInputInfo.pVertexAttributeDescriptions = _vertexDescription.attributes.data();
    _vertexInputInfo.vertexAttributeDescriptionCount = _vertexDescription.attributes.size();
    _vertexInputInfo.pVertexBindingDescriptions = _vertexDescription.bindings.data();
    _vertexInputInfo.vertexBindingDescriptionCount = _vertexDescription.bindings.size();

}

VkPipeline GraphicPipeline::build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) {

    VkPipeline pipeline;


    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; //&_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;//&_scissor;

    // std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.pNext = nullptr;
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = _dynamicStateEnables.size();
    dynamicState.pDynamicStates = _dynamicStateEnables.data();
    dynamicState.flags = 0;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.pColorBlendState = &_colorBlending;
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pDepthStencilState = &_depthStencil;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = _renderPass._renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(_device._logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        std::cout << "Failed to create graphics pipeline\n";
        return VK_NULL_HANDLE;
    }

    return pipeline;
}

VkPipeline ComputePipeline::build_pipeline(VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) {
    VkPipeline pipeline;
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.flags = 0;
    pipelineInfo.stage = shaderStages.front();

    if (vkCreateComputePipelines(_device._logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        std::cout << "Failed to create compute pipeline\n";
        return VK_NULL_HANDLE;
    }

    return pipeline;
}
