/*
*  H2Vk - CommandBuffer class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"

#include <functional>

class CommandPool;
class Device;
class Fence;
class CommandBuffer;

/** @brief command buffer recording environment */
struct UploadContext {
    /** @brief fence wrapper */
    Fence* _uploadFence;
    /** @brief command pool wrapper */
    CommandPool* _commandPool;
    /** @brief command buffer wrapper */
    CommandBuffer* _commandBuffer;
};

/**
 * CommandBuffer wrapper.
 * Command buffers are objects used to record commands which can be subsequently submitted to a device queue for execution.
 */
class CommandBuffer final {
public:
    /** @brief vulkan object used to record commands */
    VkCommandBuffer _commandBuffer;

    CommandBuffer(const Device& device, CommandPool& commandPool, uint32_t count=1);
    ~CommandBuffer();

    static void immediate_submit(const Device& device, const UploadContext& ctx, std::function<void(VkCommandBuffer cmd)>&& function);

private:
    /** @brief vulkan device wrapper */
    const class Device& _device;
    /** @brief vulkan command pool wrapper - used to manage the allocation of command buffers */
    const class CommandPool& _commandPool;
    /** @brief number of command buffers to allocate from pool */
    const uint32_t _count;
};