/*
*  H2Vk - Queue class
*
* Copyright (C) 2022-2024 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_mem_alloc.h"
#include <mutex>
#include <vector>

class Queue {
public:
    VkQueue get_queue();
    uint32_t get_queue_family();

    Queue(VkQueue queue, uint32_t family);
    Queue(const Queue&) = delete;
    Queue(Queue&&) noexcept = delete;
    ~Queue() = default;
    Queue& operator=(const Queue&) = delete;
    Queue& operator=(Queue&&) noexcept = delete;

    void queue_submit(VkPipelineStageFlags waitStage, std::vector<VkSemaphore> waitSemaphores, std::vector<VkSemaphore> signalSemaphores, std::vector<VkCommandBuffer> cmds, VkFence fence);
    void queue_submit(std::vector<VkSubmitInfo> submitInfo, VkFence fence);
    void queue_wait();
    VkResult queue_present(VkPresentInfoKHR& presentInfo);

private:
    /** @brief Device queue where command buffers are submitted to */
    VkQueue _queue;
    /** @brief Queue family : present, graphics, compute, transfer */
    uint32_t _queueFamily;
    /** @brief non-static mutex to handle multithreading. static mutex doesn't allow use of multiple queues */
    std::mutex _mutex;
};