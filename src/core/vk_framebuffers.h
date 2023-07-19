/*
*  H2Vk - A Vulkan based rendering engine
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

class FrameBuffers final {
public:
    std::vector<VkFramebuffer> _frameBuffers;

    FrameBuffers(const Window& window, const Device& device, const SwapChain& swapchain, RenderPass& renderPass);
    ~FrameBuffers();

private:
    const class Device& _device;
    const class SwapChain& _swapchain;

};