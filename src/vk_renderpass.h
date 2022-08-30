#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "VkBootstrap.h"
#include "vk_types.h"
#include "vk_swapchain.h"

class SwapChain;

class RenderPass final {
public:
    VkRenderPass _renderPass;

    RenderPass(const Device& device, SwapChain& swapChain);
    ~RenderPass();
};