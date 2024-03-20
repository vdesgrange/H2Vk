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

/**
 * Queue submit with single submit info
 * @brief thread-safe queue submit
 * @param waitStage
 * @param waitSemaphores
 * @param signalSemaphores
 * @param cmds
 * @param fence
 */
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

/**
 * @brief thread-safe queue submit
 * @param submitInfo collection of queue submit operation information
 * @param fence fence to be signaled once all submitted command buffers have completed
 */
void Queue::queue_submit(std::vector<VkSubmitInfo> submitInfo, VkFence fence) {
    std::scoped_lock<std::mutex> lock(_mutex);
    VK_CHECK(vkQueueSubmit(_queue, static_cast<uint32_t>(submitInfo.size()), submitInfo.data(), fence));
}

/**
 * @brief thread-safe wait for a queue to become idle
 */
void Queue::queue_wait() {
    std::scoped_lock<std::mutex> lock(_mutex);
    VK_CHECK(vkQueueWaitIdle(_queue));
}

/**
 * @brief queue an image for presentation. Only valid for compatible queues
 * @todo Should move to a present queue family class. Incompatible queue not handled by wrapper
 */
VkResult Queue::queue_present(VkPresentInfoKHR& presentInfo) {
    std::scoped_lock<std::mutex> lock(_mutex);
    return vkQueuePresentKHR(_queue, &presentInfo);
}