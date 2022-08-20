#pragma once

#include "vk_types.h"

namespace vkinit {
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info();
    VkPipelineColorBlendAttachmentState color_blend_attachment_state();
    VkPipelineLayoutCreateInfo pipeline_layout_create_info();
}

