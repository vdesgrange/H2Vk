#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <fstream>
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"
#include "vk_pipeline.h"
#include "vk_window.h"
#include "vk_device.h"
#include "vk_renderpass.h"
#include "core/model/vk_model.h"

/**
 * Graphics pipeline
 * graphics pipeline is the sequence of operations that take the vertices and textures of meshes all the way to the
 * pixels in the render targets.
 * vertex/index buffer -> input assembler -> vertex shader -> tesselation -> geometry shader ->
 * rasterization -> fragment shader -> color blending -> framebuffer
 * @param window
 * @param device
 * @param renderPass
 */
PipelineBuilder::PipelineBuilder(const Window& window, const Device& device, RenderPass& renderPass) :
    _device(device), _renderPass(renderPass)
{
    this->_viewport = vkinit::get_viewport((float) window._windowExtent.width, (float) window._windowExtent.height);
    this->_scissor = vkinit::get_scissor((float) window._windowExtent.width, (float) window._windowExtent.height);

    // Configure graphics pipeline - build the stage-create-info for both vertex and fragment stages
    this->_inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    this->_depthStencil = vkinit::pipeline_depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL); // VK_COMPARE_OP_LESS
    this->_rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    this->_multisampling = vkinit::multisampling_state_create_info();
    this->_colorBlendAttachment = vkinit::color_blend_attachment_state();
}

PipelineBuilder::~PipelineBuilder() {
    for (const auto& item : this->_materials) {
        vkDestroyPipeline(_device._logicalDevice, item.second->pipeline, nullptr);
        vkDestroyPipelineLayout(_device._logicalDevice, item.second->pipelineLayout, nullptr);
    }
    _materials.clear();
}

std::shared_ptr<ShaderEffect> PipelineBuilder::build_effect(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants, std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> shaderModules) {
    std::shared_ptr<ShaderEffect> effect = std::make_shared<ShaderEffect>();
    effect->setLayouts = setLayouts;
    effect->pushConstants = pushConstants;

    for (auto const& module: shaderModules) {
        VkShaderModule shader;
        if (!Shader::load_shader_module(_device, module.second, &shader)) {
            std::cout << "Error when building the shader module" << std::string(module.second) << std::endl;
        } else {
            std::cout << "Shader " << std::string(module.second) << " successfully loaded" << std::endl;
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

    pass->pipeline =  this->build_pipeline(_device, _renderPass, pass->pipelineLayout, shaderStages);

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

VkPipeline PipelineBuilder::build_pipeline(const Device& device, const RenderPass& renderPass, VkPipelineLayout& pipelineLayout, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) {

    VkPipeline pipeline;

    if (_type == Type::graphic) {
        // Warning: keep until pipeline is created because destroyed when out of scope.
        VertexInputDescription vertexDescription = Vertex::get_vertex_description();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::vertex_input_state_create_info();
        vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
        vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
        vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pNext = nullptr;
        viewportState.viewportCount = 1;
        viewportState.pViewports = NULL; //&_viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = NULL;//&_scissor;

        VkPipelineColorBlendStateCreateInfo colorBlending{}; // Dummy color blending (no transparency)
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.pNext = nullptr;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &_colorBlendAttachment;

        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.pNext = nullptr;
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = dynamicStateEnables.size();
        dynamicState.pDynamicStates = dynamicStateEnables.data();
        dynamicState.flags = 0;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
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
        pipelineInfo.renderPass = renderPass._renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device._logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            std::cout << "Failed to create graphics pipeline\n";
            return VK_NULL_HANDLE;
        }

    } else {
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.flags = 0;
        pipelineInfo.stage = shaderStages.front();

        if (vkCreateComputePipelines(device._logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            std::cout << "Failed to create compute pipeline\n";
            return VK_NULL_HANDLE;
        }
    }

    return pipeline;
}

void PipelineBuilder::create_material(const std::string &name, std::shared_ptr<Material> pass) {
    _materials.emplace(name, pass);
}

std::shared_ptr<Material> PipelineBuilder::get_material(const std::string &name) {
    auto it = _materials.find(name);
    if ( it == _materials.end()) {
        return {};
    } else {
        return it->second;
    }
}

void PipelineBuilder::scene_light(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/light/light.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/light/light.frag.spv"},
    };

    std::shared_ptr<ShaderEffect> effect = this->build_effect(setLayouts, {}, modules);
    std::shared_ptr<ShaderPass> pass = this->build_pass(effect);
    create_material("light", pass);

    for (auto& shader : effect->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_monkey_triangle(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/mesh/mesh_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/mesh/scene.frag.spv"},
    };

    VkPushConstantRange push_model;
    push_model.offset = 0;
    push_model.size = sizeof(glm::mat4);
    push_model.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange push_properties;
    push_properties.offset = sizeof(glm::mat4);
    push_properties.size = sizeof(Materials::Properties);
    push_properties.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::shared_ptr<ShaderEffect> effect_mesh = this->build_effect(setLayouts, {push_model, push_properties}, modules);
    std::shared_ptr<ShaderPass> pass_mesh = this->build_pass(effect_mesh);
    create_material("monkeyMaterial", pass_mesh);

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> pbr_modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/pbr/pbr_ibp.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/pbr/pbr_ibp.frag.spv"},
    };

    std::shared_ptr<ShaderEffect> effect_pbr = this->build_effect(setLayouts, {push_model, push_properties}, pbr_modules);
    std::shared_ptr<ShaderPass> pass_pbr = this->build_pass(effect_pbr);
    create_material("pbrMaterial", pass_pbr);

    for (auto& shader : effect_mesh->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }

    for (auto& shader : effect_pbr->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_karibu_hippo(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/mesh/mesh_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/mesh/scene_tex.frag.spv"},
    };

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(glm::mat4);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::shared_ptr<ShaderEffect> effect_mesh = this->build_effect(setLayouts, {push_constant}, modules);
    std::shared_ptr<ShaderPass> pass = this->build_pass(effect_mesh);
    create_material("karibuMaterial", pass);

    for (auto& shader : effect_mesh->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_damaged_helmet(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/mesh/mesh_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/mesh/scene_tex.frag.spv"},
    };

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(glm::mat4);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::shared_ptr<ShaderEffect> effect_mesh = this->build_effect(setLayouts, {push_constant}, modules); // {push_constant}
    std::shared_ptr<ShaderPass> pass = this->build_pass(effect_mesh);
    create_material("helmetMaterial", pass);

    for (auto& shader : effect_mesh->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }

    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> pbr_modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/pbr/pbr_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/pbr/pbr_tex.frag.spv"},
    };

    std::shared_ptr<ShaderEffect> effect_pbr = this->build_effect(setLayouts, {push_constant}, pbr_modules);
    std::shared_ptr<ShaderPass> pass_pbr = this->build_pass(effect_pbr);
    create_material("pbrTextureMaterial", pass_pbr);

    for (auto& shader : effect_pbr->shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}
