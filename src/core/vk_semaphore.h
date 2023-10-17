/*
*  H2Vk - Semaphore class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "core/utilities/vk_resources.h"

class Device;

/**
 * Synchronization primitive used to insert a dependency between queue operations
 * @brief Semaphore wrapper
 * @note Wrapper is unnecessary. Should be removed.
 */
class Semaphore final {
public:
    /** @brief Semaphore object */
    VkSemaphore _semaphore;

    explicit Semaphore(const Device& device);
    ~Semaphore();

private:
    const class Device& _device;
};