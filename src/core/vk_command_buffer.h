#pragma once

#include <GLFW/glfw3.h>
#include "VkBootstrap.h"
#include "core/utilities/vk_types.h"

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

    CommandBuffer(const Device& device, CommandPool& commandPool);
    ~CommandBuffer();

    static void immediate_submit(const Device& device, const UploadContext& ctx, std::function<void(VkCommandBuffer cmd)>&& function);
};