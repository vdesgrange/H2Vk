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
 * Framebuffer is created for a specific render pass, so RenderPass must be created first.
 * Framebuffer attachments : description of the image which will be written into the rendering commands (iow. pixels
 * are rendered on a framebuffer).
 * @brief encapsulates the state needed to setup the target for rendering, and the state of the images rendered to.
 * @note RenderPass are not necessary for compute commands.
 */
class RenderPass final {
public:

    /** @brief group sub-pass attachment related objects */
    struct Attachment {
        enum Type { color, depth, unused };
        /** @brief Abstract attachment type */
        Type type { Attachment::Type::unused };

        /** @brief Description of image written into the rendering commands (ie. color attachment) */
        VkAttachmentDescription description {};
        /** @brief Subpass which point into the attachment array when creating the render pass. */
        VkAttachmentReference ref{};
        /** @brief specify subpass dependencies (index within render pass or VK_SUBPASS_EXTERNAL otherwise) */
        VkSubpassDependency dependency{};
    };

    /** @brief vulkan device wrapper */
    VkDevice _device = VK_NULL_HANDLE;

    /** @brief render pass object, collection of attachments, subpasses, dependencies, etc. */
    VkRenderPass _renderPass = VK_NULL_HANDLE;

    RenderPass(Device& device) : _device(device._logicalDevice) {}
    ~RenderPass() {
        destroy();
    }

    RenderPass(const RenderPass&) = default;
    RenderPass& operator=(const RenderPass& other) {
        if (this != &other) { // copy-and-swap idiom
            _renderPass = other._renderPass;
            _device = other._device;
        }

        return *this;
    }

    void init(std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDependency> dependencies, std::vector<VkSubpassDescription> subpasses);
    void destroy();

    // Helpers
    static RenderPass::Attachment color(VkFormat format);
    static RenderPass::Attachment depth(VkFormat format);
    static VkSubpassDescription subpass_description(std::vector<VkAttachmentReference>& colorAttachmentRef, VkAttachmentReference* depthAttachmentRef);
};