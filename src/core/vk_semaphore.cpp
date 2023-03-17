#include "vk_semaphore.h"
#include "vk_device.h"
#include "core/utilities/vk_helpers.h"
#include "core/utilities/vk_initializers.h"

Semaphore::Semaphore(const Device &device) : _device(device) {
    //  Used for GPU -> GPU synchronisation
    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(device._logicalDevice, &semaphoreInfo, nullptr, &_semaphore));
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(_device._logicalDevice, _semaphore, nullptr);
}