#pragma once

#include "core/utilities/vk_resources.h"

class Device;

class Fence final {
public:
    VkFence _fence;

    Fence(const Device& device);
    Fence(const Device& device, VkFenceCreateFlagBits flags);
    ~Fence();

    VkResult wait(uint64_t timeout);
    VkResult reset();

private:
    const class Device& _device;
};