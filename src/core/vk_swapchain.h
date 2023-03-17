#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

#include "core/utilities/vk_resources.h"
#include "core/utilities/vk_types.h"

const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class Window;
class Device;

class SwapChain final {
public:
    VkSwapchainKHR _swapchain;
    VkFormat _swapChainImageFormat;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;

    VkFormat _depthFormat;
    VkImageView _depthImageView;
    VkImage _depthImage;
    VmaAllocation _depthAllocation;

    DeletionQueue _swapChainDeletionQueue;

    SwapChain(Window& window, const Device& device);
    ~SwapChain();

    VkExtent2D choose_swap_extent(Window& window, const VkSurfaceCapabilitiesKHR& capabilities);

private:
    const class Device& _device;
};