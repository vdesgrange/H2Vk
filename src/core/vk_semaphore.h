/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

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