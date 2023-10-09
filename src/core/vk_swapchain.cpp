/*
*  H2Vk - Swapchain class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_swapchain.h"
#include "vk_window.h"
#include "core/utilities/vk_initializers.h"

/**
 * Implementation of SwapChain
 *
 * SwapChain owns buffers where rendering happen before visualizing it.
 * A queue of images waiting to be displayed on screen.
 */
SwapChain::SwapChain(Window& window, const Device& device) : _device(device) {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device._physicalDevice, device._surface, &capabilities);

    VkExtent2D extent2d = choose_swap_extent(window, capabilities);
    window._windowExtent = extent2d;

    // uint32_t formatCount;
	// VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device._physicalDevice, device._surface, &formatCount, NULL));

	// std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	// VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device._physicalDevice, device._surface, &formatCount, surfaceFormats.data()));
    
    // VkSurfaceFormatKHR formatKHR = surfaceFormats[0];
	// std::vector<VkFormat> preferredImageFormats = { 
	// 	VK_FORMAT_B8G8R8A8_UNORM,
	// 	VK_FORMAT_R8G8B8A8_UNORM, 
	// 	VK_FORMAT_A8B8G8R8_UNORM_PACK32 
	// };

	// for (auto& availableFormat : surfaceFormats) {
	// 	if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end()) {
	// 		formatKHR = availableFormat;
	// 		break;
	// 	}
	// }

    VkSurfaceFormatKHR formatKHR{};
    formatKHR.format = VK_FORMAT_B8G8R8A8_SRGB; // VK_FORMAT_B8G8R8A8_SRGB standard RGB.
    formatKHR.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

    vkb::SwapchainBuilder swapchainBuilder{device._physicalDevice, device._logicalDevice, device._surface };
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .set_desired_format(formatKHR)  // .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(window._windowExtent.width, window._windowExtent.height)
            .build()
            .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapChainImages = vkbSwapchain.get_images().value();
    _swapChainImageViews = vkbSwapchain.get_image_views().value();
    _swapChainImageFormat = vkbSwapchain.image_format;

    VkExtent3D depthImageExtent = { // match window size
            window._windowExtent.width,
            window._windowExtent.height,
            1
    };
    _depthFormat = VK_FORMAT_D32_SFLOAT; // 32 bit

    VkImageCreateInfo imageInfo = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(device._allocator, &imageInfo, &imageAllocInfo, &_depthImage, &_depthAllocation, nullptr);

    VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(_depthFormat, _depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(device._logicalDevice, &viewInfo, nullptr, &_depthImageView));

    _swapChainDeletionQueue.push_function([=]() {
        vkDestroyImageView(device._logicalDevice, _depthImageView, nullptr);
        vmaDestroyImage(device._allocator, _depthImage, _depthAllocation);
        vkDestroySwapchainKHR(device._logicalDevice, _swapchain, nullptr);
    });
}

/**
 * Release swapchain resources
 */
SwapChain::~SwapChain() {
    // Clean up
    for (int i = 0; i < _swapChainImages.size(); i++) {
        // vkDestroyImage(_device._logicalDevice, _swapChainImages[i], nullptr); // used by framebuffer
        vkDestroyImageView(_device._logicalDevice, _swapChainImageViews[i], nullptr);
    }

    _swapChainDeletionQueue.flush();
}

/**
 * Select swapchain extent based on surface capabilities
 * @param window
 * @param capabilities
 * @return
 */
VkExtent2D SwapChain::choose_swap_extent(Window& window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window._window, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
