#pragma once

#include "VkBootstrap.h"

class Device;

class CommandPool final {
public:
    VkCommandPool _commandPool;

    CommandPool(const Device& device);
    ~CommandPool();

private:
    const class Device& _device;
};