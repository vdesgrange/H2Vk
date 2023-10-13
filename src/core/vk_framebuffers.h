/*
*  H2Vk - FrameBuffer class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_renderpass.h"
#include "core/utilities/vk_resources.h"

class Device;
class RenderPass;

/**
 * A collection of memory attachments that a render pass instance uses
 * @note Frame buffers collection created from referenced swap chain images. Framebuffer
 */
class FrameBuffer final {
public:
    /** @brief collection of frame buffers associated to render pass */
    VkFramebuffer _frameBuffer = VK_NULL_HANDLE;

    FrameBuffer() = delete;
    FrameBuffer(const RenderPass& renderPass, std::vector<VkImageView>& attachments, uint32_t width, uint32_t height, uint32_t layers);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer& rhs) : _device(rhs._renderPass._device), _renderPass(rhs._renderPass) {
        this->_frameBuffer = rhs._frameBuffer;
    }

    FrameBuffer& operator= (FrameBuffer other) = delete;

    FrameBuffer(FrameBuffer&& rhs) noexcept :  _device(rhs._renderPass._device), _renderPass(rhs._renderPass) {
        std::swap(this->_frameBuffer, rhs._frameBuffer);
    }

    FrameBuffer& operator= (FrameBuffer &&) = delete;

private:
    /** @brief Device associated to render pass */
    const class Device& _device;
    /** @brief what render pass the frame buffer will be compatible with */
    const class RenderPass& _renderPass;
};