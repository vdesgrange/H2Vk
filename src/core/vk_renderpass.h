/*
*  H2Vk - RenderPass class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vector>
#include <iostream>

#include "core/utilities/vk_resources.h"

class Device;

/**
 * RenderPass represents a collection of attachments, subpasses, dependencies between the subpasses,
 * and describes how the attachments are used over the course of the subpasses. It render into a Framebuffer.
 * @brief encapsulates the state needed to setup the target for rendering, and the state of the images rendered to.
 * @note RenderPass are not necessary for compute commands.
 */
class RenderPass final {
public:

    /** @brief group sub-pass attachment related objects */
    struct Attachment {
        /** @brief Description of image written into the rendering commands (ie. color attachment) */
        VkAttachmentDescription description {};
        /** @brief Subpass which point into the attachment array when creating the render pass. */
        VkAttachmentReference ref{};
        /** @brief specify subpass dependencies (index within render pass or VK_SUBPASS_EXTERNAL otherwise) */
        VkSubpassDependency dependency{};
    };

    /** @brief vulkan device wrapper */
    class Device& _device;

    /** @brief render pass object, collection of attachments, subpasses, dependencies, etc. */
    VkRenderPass _renderPass;

    RenderPass(Device& device);
    ~RenderPass();

    RenderPass(const RenderPass&) = default;
    RenderPass& operator=(const RenderPass& other) {
        if (this != &other) { // copy-and-swap idiom
            _renderPass = other._renderPass;
            std::swap(_device, other._device);

        }
        std::cout << "copy assigned" << std::endl;

        return *this;
    }

    void init(std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDependency> dependencies, VkSubpassDescription subpass);
    RenderPass::Attachment color(VkFormat format);
    RenderPass::Attachment depth(VkFormat format);
    VkSubpassDescription subpass_description(std::vector<VkAttachmentReference>& colorAttachmentRef, VkAttachmentReference* depthAttachmentRef);
};