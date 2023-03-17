#pragma once

#include "VkBootstrap.h"

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