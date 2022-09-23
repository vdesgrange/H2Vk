#pragma once

#include <GLFW/glfw3.h>
#include "VkBootstrap.h"
#include "vk_types.h"

class Device;

class CommandPool final {
public:
    VkCommandPool _commandPool;

    CommandPool(const Device& device);
    ~CommandPool();

private:
    const class Device& _device;
};