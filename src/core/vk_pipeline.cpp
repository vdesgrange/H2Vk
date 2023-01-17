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
#include "vk_material.h"
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
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) window._windowExtent.width;
    viewport.height = (float) window._windowExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    this->_viewport = viewport;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = window._windowExtent;
    this->_scissor = scissor;

    // Configure graphics pipeline - build the stage-create-info for both vertex and fragment stages
    this->_inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    this->_depthStencil = vkinit::pipeline_depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS);
    this->_rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    this->_multisampling = vkinit::multisampling_state_create_info();
    this->_colorBlendAttachment = vkinit::color_blend_attachment_state();
}

PipelineBuilder::~PipelineBuilder() {
//    for (const auto& item : this->_materials) {
//        vkDestroyPipeline(_device._logicalDevice, item.second->pipeline, nullptr);
//        vkDestroyPipelineLayout(_device._logicalDevice, item.second->pipelineLayout, nullptr);
//    }
    _materials.clear();

    // Duplicate with _materials
    for (const auto& item : this->_shaderPasses) {
        vkDestroyPipeline(_device._logicalDevice, item.pipeline, nullptr);
        vkDestroyPipelineLayout(_device._logicalDevice, item.pipelineLayout, nullptr);
    }
}

ShaderEffect PipelineBuilder::build_effect(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants, std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules) {
    ShaderEffect effect{};
    effect.pipelineLayout = this->build_layout(setLayouts, pushConstants); // todo useless to move it somewhere else
    effect.setLayouts = setLayouts;

    for (auto const& module: modules) {
        VkShaderModule shader;
        if (!load_shader_module(module.second, &shader)) {
            std::cout << "Error when building the shader module" << std::string(module.second) << std::endl;
        } else {
            std::cout << "Shader " << std::string(module.second) << " successfully loaded" << std::endl;
        }

        effect.shaderStages.push_back(ShaderEffect::ShaderStage{shader, module.first});
    }

    return effect;
}

VkPipelineLayout PipelineBuilder::build_layout(std::vector<VkDescriptorSetLayout> setLayouts, std::vector<VkPushConstantRange> pushConstants) {
    VkPipelineLayout layout{};
    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    layoutInfo.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();

    VK_CHECK(vkCreatePipelineLayout(_device._logicalDevice, &layoutInfo, nullptr, &layout));

    return layout;
}

ShaderPass PipelineBuilder::build_pass(ShaderEffect* effect) {
    this->set_shaders(*effect);

    ShaderPass pass{};
    pass.effect = *effect;
    pass.pipelineLayout = effect->pipelineLayout;
    pass.pipeline =  this->build_pipeline(_device, _renderPass);

    return pass;
}

void PipelineBuilder::set_shaders(ShaderEffect& effect) {
    this->_pipelineLayout = &effect.pipelineLayout;
    this->_shaderStages.clear();
    for (const auto& stage : effect.shaderStages) {
        this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(stage.flags, stage.shaderModule));
    }
}

/**
 * Create graphics pipeline
 * vertex/index buffer -> input assembler -> vertex shader -> tesselation -> geometry shader ->
 * rasterization -> fragment shader -> color blending -> framebuffer
 */
VkPipeline PipelineBuilder::build_pipeline(const Device& device, RenderPass& renderPass) {

    // Warning: keeped until pipeline is created because destroyed when out of scope.
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
    viewportState.pViewports = &_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &_scissor;

    // Dummy color blending (no transparency)
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pDepthStencilState = &_depthStencil;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.layout = *_pipelineLayout; // Comment gerer plusieurs pipeline dans une meme classe sans changer cette fonction
    pipelineInfo.renderPass = renderPass._renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device._logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cout << "Failed to create graphics pipeline\n";
        return VK_NULL_HANDLE;
    } else {
        return graphicsPipeline;
    }
}

bool PipelineBuilder::load_shader_module(const char* filePath, VkShaderModule* out) {
    const std::vector<uint32_t> code = Helper::read_file(filePath);
    return create_shader_module(code, out);
}

bool PipelineBuilder::create_shader_module(const std::vector<uint32_t>& code, VkShaderModule* out) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device._logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    *out = shaderModule;
    return true;
}

std::shared_ptr<Material> PipelineBuilder::create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string &name) {
    _materials.emplace(name, std::make_shared<Material>(pipeline, pipelineLayout));
    return _materials[name];
}

std::shared_ptr<Material> PipelineBuilder::get_material(const std::string &name) {
    auto it = _materials.find(name);
    if ( it == _materials.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

void PipelineBuilder::skybox(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/skybox/skybox.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/skybox/skybox.frag.spv"},
    };

    ShaderEffect effect = this->build_effect(setLayouts, {}, modules);
    ShaderPass pass = this->build_pass(&effect);
    this->_shaderPasses.push_back(pass);
    create_material(pass.pipeline, pass.pipelineLayout, "skyboxMaterial");

    for (auto& shader : effect.shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_light(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/light/light.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/light/light.frag.spv"},
    };

    ShaderEffect effect = this->build_effect(setLayouts, {}, modules);
    ShaderPass pass = this->build_pass(&effect);
    this->_shaderPasses.push_back(pass);
    create_material(pass.pipeline, pass.pipelineLayout, "light");

    for (auto& shader : effect.shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_monkey_triangle(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/mesh/mesh_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/mesh/scene.frag.spv"},
    };

    ShaderEffect effect_mesh = this->build_effect(setLayouts, {}, modules);
    ShaderPass pass_mesh = this->build_pass(&effect_mesh);
    this->_shaderPasses.push_back(pass_mesh);
    create_material(pass_mesh.pipeline, pass_mesh.pipelineLayout, "monkeyMaterial");

    for (auto& shader : effect_mesh.shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_karibu_hippo(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/mesh/mesh_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/mesh/scene_tex.frag.spv"},
    };

    ShaderEffect effect_mesh = this->build_effect(setLayouts, {}, modules);
    ShaderPass pass = this->build_pass(&effect_mesh);
    this->_shaderPasses.push_back(pass);
    create_material(pass.pipeline, pass.pipelineLayout, "karibuMaterial");

    for (auto& shader : effect_mesh.shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}

void PipelineBuilder::scene_damaged_helmet(std::vector<VkDescriptorSetLayout> setLayouts) {
    std::initializer_list<std::pair<VkShaderStageFlagBits, const char*>> modules {
            {VK_SHADER_STAGE_VERTEX_BIT, "../src/shaders/mesh/mesh_tex.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "../src/shaders/mesh/scene_tex.frag.spv"},
    };

    ShaderEffect effect_mesh = this->build_effect(setLayouts, {}, modules); // {push_constant}
    ShaderPass pass = this->build_pass(&effect_mesh);
    this->_shaderPasses.push_back(pass);
    create_material(pass.pipeline, pass.pipelineLayout, "helmetMaterial");

    for (auto& shader : effect_mesh.shaderStages) {
        vkDestroyShaderModule(_device._logicalDevice, shader.shaderModule, nullptr);
    }
}
