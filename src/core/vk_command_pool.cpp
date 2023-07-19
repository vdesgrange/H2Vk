/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_command_pool.h"

/**
 * Command pools manage the memory that is used to store the buffers.
 * Command buffers will be allocated from Command pools and executed on queues.
 * Commands are typically drawing operation, data transfers, etc. and need to go through command buffers.
 * @param device
 */
CommandPool::CommandPool(const Device& device) : _device(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device.get_graphics_queue_family();
    poolInfo.pNext = nullptr;

    VK_CHECK(vkCreateCommandPool(device._logicalDevice, &poolInfo, nullptr, &_commandPool));
}

CommandPool::~CommandPool() {
    if (_commandPool != nullptr) {
        vkDestroyCommandPool(_device._logicalDevice, _commandPool, nullptr);
    }
}