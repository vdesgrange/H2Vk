/*
*  H2Vk - A Vulkan based rendering engine
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

class RenderPass final {
public:

    struct Attachment {
        VkAttachmentDescription description {};
        VkAttachmentReference ref{};
        VkSubpassDependency dependency{};
    };

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

private:
    class Device& _device;
};