#include "vk_descriptor_allocator.h"
#include "vk_device.h"
#include <iostream>

DescriptorAllocator::~DescriptorAllocator() {
    for (auto p : freePools) {
        vkDestroyDescriptorPool(_device._logicalDevice, p, nullptr);
    }
    for (auto p : usedPools) {
        vkDestroyDescriptorPool(_device._logicalDevice, p, nullptr);
    }
}

bool DescriptorAllocator::allocate(VkDescriptorSet* descriptor, VkDescriptorSetLayout* setLayout, std::vector<VkDescriptorPoolSize> sizes) {
    if (_currentPool == VK_NULL_HANDLE){
        _currentPool = getPool(sizes, 0, 1000);
        usedPools.push_back(_currentPool);
    }

    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.descriptorPool = _currentPool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = setLayout;

    VkResult result = vkAllocateDescriptorSets(_device._logicalDevice, &info, descriptor);

    bool reallocate = false;
    switch (result) {
        case VK_SUCCESS:
            return true;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            reallocate = true;
            break;
        default:
            return false;
    }

    if (reallocate) {
        _currentPool = getPool(sizes, 0, 1000);
        usedPools.push_back(_currentPool);
        result = vkAllocateDescriptorSets(_device._logicalDevice, &info, descriptor);
        if (result == VK_SUCCESS){
            std::cout << "reallocate success" << std::endl;
            return true;
        }
        std::cout << "reallocate fail" << std::endl;
    }

    return false;
}

void DescriptorAllocator::resetPools() {
    for (auto p : usedPools){
        vkResetDescriptorPool(_device._logicalDevice, p, 0);
        freePools.push_back(p);
    }
    usedPools.clear();
    _currentPool = VK_NULL_HANDLE;
}

VkDescriptorPool DescriptorAllocator::getPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count) {
    if (freePools.size() > 0) {
        VkDescriptorPool pool = freePools.back();
        freePools.pop_back();
        return pool;
    }

    return createPool(sizes, flags, count);
}

VkDescriptorPool DescriptorAllocator::createPool(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t count) {
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = flags;
    info.maxSets = count;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes = sizes.data();

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(_device._logicalDevice, &info, nullptr, &descriptorPool);

    return descriptorPool;
}