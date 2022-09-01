#include "vk_command_buffer.h"
#include "vk_helpers.h"
#include "vk_device.h"
#include "vk_command_pool.h"

CommandBuffer::CommandBuffer(const Device& device, CommandPool& commandPool) {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool._commandPool;
    allocateInfo.commandBufferCount = 1; // why ? Shouldn't be a vector?
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.pNext = nullptr;

    VK_CHECK(vkAllocateCommandBuffers(device._logicalDevice, &allocateInfo, &_mainCommandBuffer));
}