#include "vk_command_buffer.h"
#include "core/utilities/vk_helpers.h"
#include "vk_device.h"
#include "vk_command_pool.h"
#include "core/utilities/vk_initializers.h"
#include "vk_fence.h"

/**
 * Command buffers are allocated from Command pools and executed on queues.
 * Commands are typically drawing operation, data transfers, etc. and need to go through command buffers.
 * Command buffers submitted to a queue can't be reset/modified until GPU finished executing them.
 * @param device
 * @param commandPool
 */
CommandBuffer::CommandBuffer(const Device& device, CommandPool& commandPool, uint32_t count) {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool._commandPool;
    allocateInfo.commandBufferCount = count;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.pNext = nullptr;

    VK_CHECK(vkAllocateCommandBuffers(device._logicalDevice, &allocateInfo, &_commandBuffer));
}

void CommandBuffer::immediate_submit(const Device& device, const UploadContext& ctx, std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBuffer cmd = ctx._commandBuffer->_commandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);
    VK_CHECK(vkQueueSubmit(device.get_graphics_queue(), 1, &submitInfo, ctx._uploadFence->_fence));

    vkWaitForFences(device._logicalDevice, 1, &ctx._uploadFence->_fence, true, 9999999999);
    vkResetFences(device._logicalDevice, 1, &ctx._uploadFence->_fence);

    vkResetCommandPool(device._logicalDevice, ctx._commandPool->_commandPool, 0);
}
