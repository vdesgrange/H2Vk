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