#pragma once

#include <vector>

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

    RenderPass(const Device& device);
    ~RenderPass();

    void init(std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDependency> dependencies, VkSubpassDescription subpass);
    RenderPass::Attachment color(VkFormat format);
    RenderPass::Attachment depth(VkFormat format);
    VkSubpassDescription subpass_description(VkAttachmentReference* colorAttachmentRef, VkAttachmentReference* depthAttachmentRef);

private:
    const class Device& _device;
};