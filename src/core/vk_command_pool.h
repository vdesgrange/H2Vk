/*
*  H2Vk - CommandPool class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"

class Device;

/**
 * Command pools are opaque objects that command buffer memory is allocated from.
 * @brief vulkan command pool wrapper
 * @note mostly sugar syntax
 */
class CommandPool final {
public:
    /** @brief vulkan command pool object */
    VkCommandPool _commandPool;

    explicit CommandPool(const Device& device);
    ~CommandPool();

private:
    /** @brief vulkan device wrapper */
    const class Device& _device;
};