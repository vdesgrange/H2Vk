#include "vk_fence.h"
#include "vk_helpers.h"
#include "vk_initializers.h"
#include "vk_device.h"

Fence::Fence(const Device& device) : _device(device) {
    // Used for CPU -> GPU communication
    VkFenceCreateInfo fenceInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(device._logicalDevice, &fenceInfo, nullptr, &_fence));

//    _mainDeletionQueue.push_function([=]() { // Destruction of fence
//        vkDestroyFence(device._logicalDevice, _renderFence, nullptr);
//    });
}

Fence::~Fence() {
    vkDestroyFence(_device._logicalDevice, _fence, nullptr);
}

VkResult Fence::wait(uint64_t timeout=1000000000) {
    return vkWaitForFences(_device._logicalDevice, 1, &_fence, VK_TRUE, timeout);
}

VkResult Fence::reset() {
    return vkResetFences(_device._logicalDevice, 1, &_fence);
}