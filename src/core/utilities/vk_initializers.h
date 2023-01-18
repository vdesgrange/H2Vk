#pragma once

#include "vk_types.h"

namespace vkinit {
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);
    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
    VkPipelineColorBlendAttachmentState color_blend_attachment_state();
    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);
    VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer frameBuffer);
    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags flags);
    VkPipelineLayoutCreateInfo pipeline_layout_create_info();
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
    VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags flags, uint32_t binding);
    VkWriteDescriptorSet write_descriptor_set(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* binfo, uint32_t binding);
    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
    VkSubmitInfo submit_info(VkCommandBuffer* cmd);
    VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
    VkViewport get_viewport(float width, float height);
    VkRect2D get_scissor(float width, float height);
}

