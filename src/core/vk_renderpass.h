#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "VkBootstrap.h"
#include "core/utilities/vk_types.h"
#include "vk_swapchain.h"

class SwapChain;
class Device;

class RenderPass final {
public:
    VkRenderPass _renderPass;

    RenderPass(const Device& device, SwapChain& swapChain);
    ~RenderPass();

private:
    const class Device& _device;
};