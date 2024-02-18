/*
*  H2Vk - CommandBuffer class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_command_buffer.h"
#include "vk_command_pool.h"
#include "vk_fence.h"
#include "core/utilities/vk_initializers.h"

/**
 * Command buffers are allocated from Command pools and executed on queues.
 * Commands are typically drawing operation, data transfers, etc. and need to go through command buffers.
 * Command buffers submitted to a queue can't be reset/modified until GPU finished executing them.
 * @param device vulkan device wrapper
 * @param commandPool
 * @param count number of command buffer to allocate from the pool
 */
CommandBuffer::CommandBuffer(const Device& device, CommandPool& commandPool, uint32_t count) : _device(device), _commandPool(commandPool), _count(count) {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool._commandPool;
    allocateInfo.commandBufferCount = count;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.pNext = nullptr;

    VK_CHECK(vkAllocateCommandBuffers(device._logicalDevice, &allocateInfo, &_commandBuffer));
}

/**
 * Free all command buffers which were allocated from the pool
 * @brief default destructor
 */
CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(_device._logicalDevice, _commandPool._commandPool, _count, &_commandBuffer);
}

void CommandBuffer::record(std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    std::scoped_lock<std::mutex> lock(this->_mutex);
    VK_CHECK(vkBeginCommandBuffer(this->_commandBuffer, &cmdBeginInfo));
    function(this->_commandBuffer);
    VK_CHECK(vkEndCommandBuffer(this->_commandBuffer));
}

VkCommandBuffer CommandBuffer::begin() {
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    this->_mutex.lock();
    VK_CHECK(vkBeginCommandBuffer(this->_commandBuffer, &cmdBeginInfo));

    return this->_commandBuffer;
}

VkCommandBuffer CommandBuffer::end() {
    VK_CHECK(vkEndCommandBuffer(this->_commandBuffer)); // Finish recording a command buffer
    this->_mutex.unlock();

    return this->_commandBuffer;
}

/**
 * Will records a command (defined as a lambda) and
 * @brief records and submit command for the GPU
 * @param device vulkan device wrapper
 * @param ctx executing command buffer and fences it triggers
 * @param function lambda executed by the command buffer
 */
void CommandBuffer::immediate_submit(const Device& device, const UploadContext& ctx, std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBuffer cmd = ctx._commandBuffer->_commandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    std::scoped_lock<std::mutex> lock(ctx._commandBuffer->_mutex);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo)); // Start recording command buffer
    function(cmd); // Commands we want to execute into a command buffer
    VK_CHECK(vkEndCommandBuffer(cmd)); // Finish recording a command buffer

    std::vector<VkSubmitInfo> submitInfo = {vkinit::submit_info(&cmd)}; // queue submit op. specifications
    // Submits a sequence of semaphores or command buffers to the graphics queue. todo - Allow choice of queue

    device._queue->queue_submit(submitInfo, ctx._uploadFence->_fence);

    // Wait for one or more fences to become signaled
    vkWaitForFences(device._logicalDevice, 1, &ctx._uploadFence->_fence, true, 9999999999);
    vkResetFences(device._logicalDevice, 1, &ctx._uploadFence->_fence);

    // Recycles the resources from the command buffers allocated from the command pool back to the command pool.
    vkResetCommandPool(device._logicalDevice, ctx._commandPool->_commandPool, 0);
}
