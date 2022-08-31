#include "vk_semaphore.h"
#include "vk_helpers.h"
#include "vk_device.h"
#include "vk_initializers.h"

Semaphore::Semaphore(const Device &device) : _device(device) {
    //  Used for GPU -> GPU synchronisation
    VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(device._logicalDevice, &semaphoreInfo, nullptr, &_semaphore));

//    _mainDeletionQueue.push_function([=]() { // Destruction of semaphores
//        vkDestroySemaphore(device._logicalDevice, _presentSemaphore, nullptr);
//        vkDestroySemaphore(device._logicalDevice, _renderSemaphore, nullptr);
//    });
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(_device._logicalDevice, _semaphore, nullptr);
}