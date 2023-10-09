/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_initializers.h"

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, VkSpecializationInfo* pShaderSpe) {
    /**
     * Hold information about a single shader stage for the pipeline.
     * Build from a shader stage and a shader module.
     */
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.stage = stage;
    info.module = shaderModule;
    info.pName = "main";
    info.pSpecializationInfo = pShaderSpe;

    return info;
}

/**
 * @brief Information for vertex buffers and vertex formats
 */
VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.vertexBindingDescriptionCount = 0;
    info.pVertexBindingDescriptions = nullptr;
    info.vertexAttributeDescriptionCount = 0;
    info.pVertexAttributeDescriptions = nullptr;

    return info;
}

/**
 *  @brief Configuration for the kind of topology which will be drawn
 *  @param topology triangle, lines, points drawing
 *  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_POINT_LIST
 */
VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = topology;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
}

/**
 *  @brief Configuration for the fixed-function rasterization
 *  Enable or disable backface culling, set line width or wireframe drawing.
 *  @param polygonMode
 */
VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(VkPolygonMode polygonMode) {
    VkPipelineRasterizationStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode = polygonMode;
    info.lineWidth = 1.0f;
    info.cullMode = VK_CULL_MODE_NONE;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;

    return info;
}

/**
 *  @brief Allows to configure multisample anti-aliasing (MSAA) for the pipeline
 *  no multisampling = 1 sample per pixel
 */
VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info() {
    VkPipelineMultisampleStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.sampleShadingEnable = VK_FALSE;
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // no multisampling = 1 sample per pixel
    info.minSampleShading = 1.0f;
    info.pSampleMask = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
}

/**
 * @brief Controls how the pipeline blends into a given attachment
 * Rendering to 1 attachment = 1 color blend attachment state
 * blend disabled = override
 */
VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    return colorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo vkinit::color_blend_state_create_info(VkPipelineColorBlendAttachmentState* colorBlendAttachment) {
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = colorBlendAttachment;

    return colorBlending;
}

/**
 * @brief create fence info structure
 * @note Used for CPU -> GPU communication
 * @param flags
 * @return
 */
VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;

    return info;
}

/**
 * @brief create semaphore info structure
 * @note Used for GPU -> GPU synchronisation
 * @param flags
 * @return
 */
VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;

    return info;
}

VkRenderPassBeginInfo vkinit::renderpass_begin_info(VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer frameBuffer) {
    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.pNext = nullptr;
    info.renderPass = renderPass;
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = extent;
    info.framebuffer = frameBuffer; // Which image to render from the swapchain
    info.clearValueCount = 1;
    info.pClearValues = nullptr;

    return info;
}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent) {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.format = format;
    info.usage = flags;
    info.extent = extent;
    info.imageType = VK_IMAGE_TYPE_2D;  // Hold dimensionality of the texture
    info.arrayLayers = 1;
    info.mipLevels = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return info;
}

VkImageViewCreateInfo vkinit::imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags flags) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;  // More option than imageType. Ex. cube mapping (different from type 3d)
    info.format = format; // Must match Image format
    info.image = image;
    info.subresourceRange.aspectMask = flags;  // What is the image used for
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;

    return info;
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // default
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;

    return info;
}

VkPipelineDepthStencilStateCreateInfo vkinit::pipeline_depth_stencil_state_create_info(bool bDepthTest,
                                                                                       bool bDepthWrite,
                                                                                       VkCompareOp compareOp) {
    VkPipelineDepthStencilStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE; // z-culling = correct reproduction of depth perception:close object hides one further away
    info.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE;
    info.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
    info.back.compareOp = VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.stencilTestEnable = VK_FALSE;

    return info;
}

VkDescriptorSetLayoutBinding vkinit::descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags flags, uint32_t binding) {
    VkDescriptorSetLayoutBinding setBind{};
    setBind.binding = binding;
    setBind.descriptorCount = 1;
    setBind.descriptorType = type;
    setBind.stageFlags = flags;
    setBind.pImmutableSamplers = nullptr;

    return setBind;
}

/**
 * initialize VkWriteDescriptorSet structure
 * @param type descriptor type
 * @param dstSet destination
 * @param binfo pointer to buffer info
 * @param binding index of binding
 * @return
 */
VkWriteDescriptorSet vkinit::write_descriptor_set(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* binfo, uint32_t binding) {
    VkWriteDescriptorSet setWrite = {};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.pNext = nullptr;
    setWrite.descriptorCount = 1;
    setWrite.descriptorType = type;
    setWrite.dstBinding = binding;
    setWrite.dstSet = dstSet;
    setWrite.pBufferInfo = binfo;

    return setWrite;
}

/**
 * Define additional information about how the command buffer begins recording
 * @brief Initialize command buffer begin info
 * @param flags usage behavior for the command buffer
 * @return begin operation specification
 */
VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;
    info.pInheritanceInfo = nullptr;
    info.flags = flags;
    return info;
}

/**
 * Specify queue submit operation informations.
 * Number of semaphores upon which to wait or to signal before executing the command buffer,
 * number of command buffers to execute
 * @param cmd pointer to an array of VkCommandBuffer to execute in batch
 * @return queue submit specification
 */
VkSubmitInfo vkinit::submit_info(VkCommandBuffer* cmd) {
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext = nullptr;
    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = nullptr;
    info.pWaitDstStageMask = nullptr;
    info.commandBufferCount = 1;
    info.pCommandBuffers = cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores = nullptr;
    return info;
}

/**
 * @brief initialize VkSamplerCreateInfo structure
 * @param filters which filter applies for lookup (minification & magnification)
 * @param samplerAddressMode how UVW (XYZ) outside coordinates are handled
 * @return
 */
VkSamplerCreateInfo vkinit::sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = nullptr;
    info.magFilter = filters;
    info.minFilter = filters;
    info.addressModeU = samplerAddressMode;
    info.addressModeV = samplerAddressMode;
    info.addressModeW = samplerAddressMode;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;
    info.mipLodBias = 0.0f;
    info.maxAnisotropy = 0.0f;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_NEVER;

    return info;
}

/**
 * @brief initialize VkWriteDescriptorSet structure
 * @param type type
 * @param dstSet destination
 * @param imageInfo pointer to image info (sampler, view, layout)
 * @param binding binding index
 * @return
 */
VkWriteDescriptorSet vkinit::write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding)
{
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imageInfo;
    return write;
}

/**
 * @brief initialize viewport
 * @param width
 * @param height
 * @return viewport
 */
VkViewport vkinit::get_viewport(float width, float height) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}

/**
 * @brief initalize two-dimensional sub region structure
 * @param width
 * @param height
 * @return VkRect2D
 */
VkRect2D vkinit::get_scissor(float width, float height) {
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent.width = width;
    scissor.extent.height = height;

    return scissor;
}