/*
*  H2Vk - FrameBuffer class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_framebuffers.h"
#include "vk_window.h"
#include "vk_renderpass.h"

/**
 * A collection of memory attachments that a render pass instance uses.
 *
 * @brief framebuffer constructor
 * @param renderPass render pass associated to the framebuffer
 * @param attachments collection of image view handles used as corresponding attachments in a render pass instance.
 * @param width, height, layers framebuffer dimensions
 */
FrameBuffer::FrameBuffer(const RenderPass& renderPass, std::vector<VkImageView>& attachments, uint32_t width, uint32_t height, uint32_t layers) : _renderPass(renderPass) {

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass._renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = layers;

    VK_CHECK(vkCreateFramebuffer(_renderPass._device._logicalDevice, &framebufferInfo, nullptr, &_frameBuffer));
}

/**
 * @brief Move constructor
 * @param rhs other object
 * @note Rule of five, vector requires move semantic.
 */
FrameBuffer::FrameBuffer(FrameBuffer&& rhs) noexcept : _renderPass(rhs._renderPass) {
    this->_frameBuffer = rhs._frameBuffer;
    rhs._frameBuffer = nullptr;
    // std::swap(this->_frameBuffer, rhs._frameBuffer);
}

/**
 * release frame buffers resources used by associated swap chain
 * @brief default destructor
 */
FrameBuffer::~FrameBuffer() {
    if (_frameBuffer != nullptr) { // By default VkFrameBuffer might simply be invalid instead of null pointer.
        vkDestroyFramebuffer(_renderPass._device._logicalDevice, _frameBuffer, nullptr);
    }
}