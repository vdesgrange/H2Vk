/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_fence.h"
#include "core/utilities/vk_initializers.h"

/**
 * @brief constructor
 * @param device vulkan device wrapper
 */
Fence::Fence(const Device& device) : _device(device) {
    // Used for CPU -> GPU communication
    VkFenceCreateInfo fenceInfo = vkinit::fence_create_info();
    VK_CHECK(vkCreateFence(device._logicalDevice, &fenceInfo, nullptr, &_fence));
}

/**
 * @brief constructor
 * @param device vulkan device wrapper
 * @param flags specifies that the fence object is created in the signaled state.
 */
Fence::Fence(const Device& device, VkFenceCreateFlagBits flags) : _device(device) {
    // Used for CPU -> GPU communication
    VkFenceCreateInfo fenceInfo = vkinit::fence_create_info(flags); // VK_FENCE_CREATE_SIGNALED_BIT
    VK_CHECK(vkCreateFence(device._logicalDevice, &fenceInfo, nullptr, &_fence));
}

/**
 * @brief default destructor
 */
Fence::~Fence() {
    vkDestroyFence(_device._logicalDevice, _fence, nullptr);
}

/**
 * @brief wait for one or more fences to become signaled
 * @param timeout
 * @return true if successful
 */
VkResult Fence::wait(uint64_t timeout=1000000000) {
    return vkWaitForFences(_device._logicalDevice, 1, &_fence, VK_TRUE, timeout);
}

/**
 * @brief reset one or more fences
 * @return true if successful
 */
VkResult Fence::reset() {
    return vkResetFences(_device._logicalDevice, 1, &_fence);
}