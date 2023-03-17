#pragma once

#include "core/utilities/vk_resources.h"

class Device;

class Semaphore final {
public:
    VkSemaphore _semaphore;

    Semaphore(const Device& device);
    ~Semaphore();

private:
    const class Device& _device;
};