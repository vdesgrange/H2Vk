#include "vk_descriptor_pools.h"
#include "vk_device.h"

DescriptorPools::DescriptorPools(const Device& device, std::vector<VkDescriptorPoolSize> sizes) : _device(device) {

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool_info.pPoolSizes = sizes.data();

    vkCreateDescriptorPool(device._logicalDevice, &pool_info, nullptr, &_descriptorPool);
}

DescriptorPools::~DescriptorPools() {
    if (_descriptorPool != nullptr) {
        vkDestroyDescriptorPool(_device._logicalDevice, _descriptorPool, nullptr);
        _descriptorPool = nullptr;
    }
}