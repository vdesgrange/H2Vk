/*
*  H2Vk - Fence class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"

class Device;

/**
 * Synchronization primitive used to insert a dependency from a queue to the host
 * @brief Fence wrapper
 * @note Wrapper is unnecessary. Should be removed.
 */
class Fence final {
public:
    /** @brief  Fence object */
    VkFence _fence;

    Fence(const Device& device);
    Fence(const Device& device, VkFenceCreateFlagBits flags);
    ~Fence();

    VkResult wait(uint64_t timeout);
    VkResult reset();

private:
    /** @brief vulkan device wrapper */
    const class Device& _device;
};