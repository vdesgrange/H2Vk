#include "vk_framebuffers.h"
#include "core/utilities/vk_helpers.h"
#include "vk_window.h"
#include "vk_device.h"
#include "vk_renderpass.h"
#include "vk_swapchain.h"

/**
 * Framebuffer object references all of the VkImageView objects that represent the attachments (ie. color)
 * Image that we have to use for the attachment depends on which image the swap chain returns,
 * a framebuffer must be created all of the images in the swap chain.
 * @param window
 * @param device
 * @param swapchain
 * @param renderPass
 */
FrameBuffers::FrameBuffers(const Window& window, const Device& device, const SwapChain& swapchain, RenderPass& renderPass) : _device(device), _swapchain(swapchain) {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = window._windowExtent.width;
    framebufferInfo.height = window._windowExtent.height;
    framebufferInfo.layers = 1;

    _frameBuffers.resize(swapchain._swapChainImages.size());

    for (int i = 0; i < swapchain._swapChainImages.size(); i++) {
        std::array<VkImageView, 2> attachments = {swapchain._swapChainImageViews[i], swapchain._depthImageView};
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();

        VK_CHECK(vkCreateFramebuffer(device._logicalDevice, &framebufferInfo, nullptr, &_frameBuffers[i]));
    }
}

FrameBuffers::~FrameBuffers() {
    for (int i = 0; i < _swapchain._swapChainImages.size(); i++) {
        vkDestroyFramebuffer(_device._logicalDevice, _frameBuffers[i], nullptr);
        // vkDestroyImageView(_device._logicalDevice, _swapchain._swapChainImageViews[i], nullptr);
    }
}