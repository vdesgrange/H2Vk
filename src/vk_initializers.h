﻿#pragma once

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

    VkPipelineLayoutCreateInfo pipeline_layout_create_info();
}

