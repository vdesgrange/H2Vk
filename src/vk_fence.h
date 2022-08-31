#pragma once

#include "VkBootstrap.h"

class Device;

class Fence final {
public:
    VkFence _fence;

    Fence(const Device& device);
    ~Fence();

    VkResult wait(uint64_t timeout);
    VkResult reset();

private:
    const class Device& _device;
};