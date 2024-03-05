/*
*  H2Vk - Queue class
*
* Copyright (C) 2022-2024 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_queue.h"
#include "core/utilities/vk_helpers.h"


Queue::Queue(VkQueue queue, uint32_t family) : _queue(queue), _queueFamily(family) {}

/**
 * Get queue pointer.
 * Assume to wrap both present and graphics operation. Single graphics queue for basic GPU.
 * @return
 */
VkQueue Queue::get_queue() {
    return _queue;
}

/**
 * Get queue family index
 * @return
 */
uint32_t Queue::get_queue_family() {
    return _queueFamily;
}

void Queue::queue_submit(VkPipelineStageFlags waitStage, std::vector<VkSemaphore> waitSemaphores, std::vector<VkSemaphore> signalSemaphores, std::vector<VkCommandBuffer> cmds, VkFence fence) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()); // Semaphore to wait before executing the command buffers
    submitInfo.pWaitSemaphores = waitSemaphores.data(); // Wait acquisition of next presentable image
    submitInfo.signalSemaphoreCount =  static_cast<uint32_t>(signalSemaphores.size()); // Number of semaphores to be signaled once the commands are executed
    submitInfo.pSignalSemaphores = signalSemaphores.data(); // Signal image is rendered and ready to be presented
    submitInfo.commandBufferCount = static_cast<uint32_t>(cmds.size()); // Number of command buffers to execute in the batch
    submitInfo.pCommandBuffers = cmds.data();

    std::scoped_lock<std::mutex> lock(_mutex);
    VK_CHECK(vkQueueSubmit(_queue, 1, &submitInfo, fence));
}

void Queue::queue_submit(std::vector<VkSubmitInfo> submitInfo, VkFence fence) {
    std::scoped_lock<std::mutex> lock(_mutex);
    VK_CHECK(vkQueueSubmit(_queue, static_cast<uint32_t>(submitInfo.size()), submitInfo.data(), fence));
}

void Queue::queue_wait() {
    VK_CHECK(vkQueueWaitIdle(_queue));
}