#pragma once

#include <GLFW/glfw3.h>
#include "VkBootstrap.h"
#include "vk_types.h"

class CommandPool;
class Device;

class CommandBuffer final {
public:
    VkCommandBuffer _mainCommandBuffer;

    CommandBuffer(const Device& device, CommandPool& commandPool);
    ~CommandBuffer();
};