#include "vk_command_pool.h"
#include "vk_device.h"
#include "vk_helpers.h"

CommandPool::CommandPool(const Device& device, DeletionQueue& mainDeletionQueue) : _device(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device.get_graphics_queue_family();
    poolInfo.pNext = nullptr;

    VK_CHECK(vkCreateCommandPool(device._logicalDevice, &poolInfo, nullptr, &_commandPool));

    // mainDeletionQueue - might not be required
//    mainDeletionQueue.push_function([=]() {
//        vkDestroyCommandPool(device._logicalDevice, _commandPool, nullptr);
//    });
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(_device._logicalDevice, _commandPool, nullptr);
}