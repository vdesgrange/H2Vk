#pragma once

#include "VkBootstrap.h"

class Device;

class Semaphore final {
public:
    VkSemaphore _semaphore;

    Semaphore(const Device& device);
    ~Semaphore();

private:
    const class Device& _device;
};