/*
*  H2Vk - RenderPass class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_renderpass.h"

/**
 * RenderPass
 * Render pass object wraps all information related to the framebuffer attachments that will be used for rendering.
 * Framebuffers are created for a specific render pass, so RenderPass must be created first.
 * Framebuffer attachments : description of the image which will be written into the rendering commands (iow. pixels
 * are rendered on a framebuffer).
 * @brief constructor
 * @param device vulkan device wrapper
 */
RenderPass::RenderPass(Device& device) : _device(device), _renderPass(VK_NULL_HANDLE) {
}

/**
 * Destroy render pass
 * @brief default destructor
 */
RenderPass::~RenderPass() {
    vkDestroyRenderPass(_device._logicalDevice, _renderPass, nullptr);
}


/**
 * Initialize a render pass and a single sub-pass with a collection of attachments and dependencies
 * @brief initialize render pass with unique sub-pass
 * @note todo - reorganize : color and depth attachment are a mess
 * @param attachments collection of sub-pass descriptions
 * @param dependencies collection of sub-pass dependencies
 * @param subpass single sub-pass of the render pass
 */
void RenderPass::init(std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDependency> dependencies, VkSubpassDescription subpass) {
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(_device._logicalDevice, &renderPassInfo, nullptr, &_renderPass));
}

/**
 * Create structure for color attachment, description of the image which will be writing into with rendering commands
 * @brief Initialize color attachment objects
 * @param format color attachment format
 * @return structure for sub-pass color attachment related objects
 */
RenderPass::Attachment RenderPass::color(VkFormat format) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    return RenderPass::Attachment {colorAttachment, colorAttachmentRef, dependency};
}

/**
 * Create structure for depth buffer which will allow z-testing for rendering of 3d assets.
 * @brief Initialize depth attachment objects
 * @param format depth format
 * @return structure for sub-pass depth attachment related objects
 */
RenderPass::Attachment RenderPass::depth(VkFormat format) {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = format;
    depthAttachment.flags = 0;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency depthDependency{};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    return RenderPass::Attachment {depthAttachment, depthAttachmentRef,depthDependency};
}

/**
 * @brief Create sub-pass attachment description
 * @param colorAttachmentRef collection of color attachments, images which will be written into by rendering commands
 * @param depthAttachmentRef one depth attachment
 * @return sub-pass description
 */
VkSubpassDescription RenderPass::subpass_description(std::vector<VkAttachmentReference>& colorAttachmentRef, VkAttachmentReference* depthAttachmentRef) {
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRef.size());
    subpass.pColorAttachments = colorAttachmentRef.data(); // link color attachments to sub-pass (fragment can write to multiple color attachments)
    subpass.pDepthStencilAttachment = depthAttachmentRef; // link depth attachment to sub-pass

    return subpass;
}