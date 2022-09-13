#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <fstream>
#include "vk_helpers.h"
#include "vk_initializers.h"
#include "vk_mesh.h"
#include "vk_pipeline.h"
#include "vk_window.h"
#include "vk_device.h"
#include "vk_renderpass.h"
#include "vk_material.h"

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
PipelineBuilder::PipelineBuilder(const Window& window, const Device& device, RenderPass& renderPass, VkDescriptorSetLayout& globalSetLayout) :
    _device(device)
{

    // Load shaders
    VkShaderModule triangleFragShader;
    if (!load_shader_module("../shaders/default_lit.frag.spv", &triangleFragShader))
    {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    }
    else {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangleVertexShader;
    if (!load_shader_module("../shaders/shader_base.vert.spv", &triangleVertexShader))
    {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;

    }
    else {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    // Load shaders
    VkShaderModule redTriangleFragShader;
    if (!load_shader_module("../shaders/red_shader_base.frag.spv", &redTriangleFragShader))
    {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    }
    else {
        std::cout << "Red triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule redTriangleVertexShader;
    if (!load_shader_module("../shaders/red_shader_base.vert.spv", &redTriangleVertexShader))
    {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;

    }
    else {
        std::cout << "Red triangle vertex shader successfully loaded" << std::endl;
    }

    VkShaderModule meshVertShader;
    if (!load_shader_module("../shaders/tri_mesh.vert.spv", &meshVertShader)) {
        std::cout << "Error when building the green triangle mesh vertex shader module" << std::endl;
    }
    else {
        std::cout << "Green triangle vertex shader successfully loaded" << std::endl;
    }


    // Build pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &globalSetLayout;
    //VkPipelineLayout _triPipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device._logicalDevice, &pipeline_layout_info, nullptr, &_triPipelineLayout));
    this->_pipelineLayout = &_triPipelineLayout;

    // Configure graphics pipeline - build the stage-create-info for both vertex and fragment stages
    this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
    this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
    this->_vertexInputInfo = vkinit::vertex_input_state_create_info();
    this->_inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    this->_depthStencil = vkinit::pipeline_depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS);

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

    this->_rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    this->_multisampling = vkinit::multisampling_state_create_info();
    this->_colorBlendAttachment = vkinit::color_blend_attachment_state();

    // === 1 - Build graphics pipeline ===
    _graphicsPipeline = this->build_pipeline(device, renderPass);

    // === 2 - Build red triangle pipeline ===
    this->_shaderStages.clear();
    this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertexShader));
    this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
    _redTrianglePipeline = this->build_pipeline(device, renderPass);

    // === 3 - Build dynamic triangle mesh
    this->_shaderStages.clear();  // Clear the shader stages for the builder

    VertexInputDescription vertexDescription = Vertex::get_vertex_description();

    // Connect the pipeline builder vertex input info to the one we get from Vertex
    this->_vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    this->_vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    this->_vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    this->_vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

    this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
    this->_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = static_cast<uint32_t>(sizeof(MeshPushConstants));
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
    mesh_pipeline_layout_info.pushConstantRangeCount = 1;
    mesh_pipeline_layout_info.setLayoutCount = 1;
    mesh_pipeline_layout_info.pSetLayouts = &globalSetLayout;

    //VkPipelineLayout meshPipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device._logicalDevice, &mesh_pipeline_layout_info, nullptr, &_meshPipelineLayout));
    this->_pipelineLayout = &_meshPipelineLayout;

    _meshPipeline = this->build_pipeline(device, renderPass);
    create_material(_meshPipeline, _meshPipelineLayout, "defaultMesh");

    // === 4 - Clean
    // Deleting shaders
    vkDestroyShaderModule(device._logicalDevice, redTriangleVertexShader, nullptr);
    vkDestroyShaderModule(device._logicalDevice, redTriangleFragShader, nullptr);
    vkDestroyShaderModule(device._logicalDevice, triangleFragShader, nullptr);
    vkDestroyShaderModule(device._logicalDevice, triangleVertexShader, nullptr);
    vkDestroyShaderModule(device._logicalDevice, meshVertShader, nullptr);

}

PipelineBuilder::~PipelineBuilder() {
    _materials.clear();
    vkDestroyPipeline(_device._logicalDevice, _redTrianglePipeline, nullptr);
    vkDestroyPipeline(_device._logicalDevice, _graphicsPipeline, nullptr);
    vkDestroyPipeline(_device._logicalDevice, _meshPipeline, nullptr);
    vkDestroyPipelineLayout(_device._logicalDevice, _triPipelineLayout, nullptr);
    vkDestroyPipelineLayout(_device._logicalDevice, _meshPipelineLayout, nullptr);
    // vkDestroyPipelineLayout(_device._logicalDevice, _pipelineLayout, nullptr); // attention !
}

/**
 * Create graphics pipeline
 * vertex/index buffer -> input assembler -> vertex shader -> tesselation -> geometry shader ->
 * rasterization -> fragment shader -> color blending -> framebuffer
 */
VkPipeline PipelineBuilder::build_pipeline(const Device& device, RenderPass& renderPass) {
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

    pipelineInfo.stageCount = _shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = *_pipelineLayout; // Comment gerer plusieurs pipeline dans une meme classe sans changer cette fonction
    pipelineInfo.renderPass = renderPass._renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &_depthStencil;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device._logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipeline\n";
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

Material* PipelineBuilder::create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string &name) {
    Material mat(pipeline, pipelineLayout);
    _materials[name] = mat;
    return &_materials[name];
}

Material* PipelineBuilder::get_material(const std::string &name) {
    auto it = _materials.find(name);
    if ( it == _materials.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}
