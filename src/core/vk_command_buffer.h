/*
*  H2Vk - A Vulkan based rendering engine
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

struct UploadContext {
    Fence* _uploadFence;
    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;
};

class CommandBuffer final {
public:
    VkCommandBuffer _commandBuffer;

    CommandBuffer(const Device& device, CommandPool& commandPool, uint32_t count=1);
    ~CommandBuffer();

    static void immediate_submit(const Device& device, const UploadContext& ctx, std::function<void(VkCommandBuffer cmd)>&& function);

private:
    const class Device& _device;
    const class CommandPool& _commandPool;
    const uint32_t _count;
};