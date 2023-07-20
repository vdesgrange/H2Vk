/*
*  H2Vk - Framebuffers class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"

class Window;
class Device;
class SwapChain;
class RenderPass;

/**
 * A collection of memory attachments that a render pass instance uses
 * @note Frame buffers collection created from referenced swap chain images. Framebuffer
 */
class FrameBuffers final {
public:
    /** @brief collection of frame buffers associated to render pass */
    std::vector<VkFramebuffer> _frameBuffers;

    FrameBuffers(const Window& window, const Device& device, const SwapChain& swapchain, RenderPass& renderPass);
    ~FrameBuffers();

private:
    /** @brief vulkan device wrapper */
    const class Device& _device;
    /** @brief swap chain whose images are used to create frame buffers */
    const class SwapChain& _swapchain;

};