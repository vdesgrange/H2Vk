/*
*  H2Vk - Swapchain class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

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

/**
 * @brief Swapchain is an abstraction for an array of presentable images that are associated with a surface.
 */
class SwapChain final {
public:
    /** @brief Swapchain object used to present rendering results to a surface */
    VkSwapchainKHR _swapchain;
    /** @brief Swapchain image format. */
    VkFormat _swapChainImageFormat;
    /** @brief Presentable images. One image displayed at a time. */
    std::vector<VkImage> _swapChainImages;
    /** @brief Presentable images views. */
    std::vector<VkImageView> _swapChainImageViews;
    /** @brief depth image format */
    VkFormat _depthFormat;
    /** @brief depth view format */
    VkImageView _depthImageView;
    /** @brief depth image */
    VkImage _depthImage;
    /** @brief depth image memory allocation */
    VmaAllocation _depthAllocation;
    /** @brief swapchain clean queue. Call at the end. */
    DeletionQueue _swapChainDeletionQueue;

    SwapChain(Window& window, const Device& device);
    ~SwapChain();

    VkExtent2D choose_swap_extent(Window& window, const VkSurfaceCapabilitiesKHR& capabilities);

private:
    /** @brief device class */
    const class Device& _device;
};