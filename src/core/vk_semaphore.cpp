/*
*  H2Vk - Semaphone class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_semaphore.h"
#include "core/utilities/vk_initializers.h"

/**
 * @brief semaphore constructor
 * @param device vulkan device wrappr
 */
Semaphore::Semaphore(const Device &device) : _device(device) {
    //  Used for GPU -> GPU synchronisation
    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(device._logicalDevice, &semaphoreInfo, nullptr, &_semaphore));
}

/**
 * @brief semaphore destructor
 */
Semaphore::~Semaphore() {
    vkDestroySemaphore(_device._logicalDevice, _semaphore, nullptr);
}