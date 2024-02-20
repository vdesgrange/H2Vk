/*
*  H2Vk - DescriptorAllocator class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_descriptor_allocator.h"
#include <iostream>

/**
 * @brief default destructor
 * destroy descriptor pools, free and invalidate descriptor sets allocated from the pools.
 */
DescriptorAllocator::~DescriptorAllocator() {
    for (auto p : freePools) {
        vkDestroyDescriptorPool(_device._logicalDevice, p, nullptr);
    }
    for (auto p : usedPools) {
        vkDestroyDescriptorPool(_device._logicalDevice, p, nullptr);
    }
}

/**
 * Allocate a descriptor set.
 * Get an unused descriptor pool or create a new one. Allocate a single descriptor set from the descriptor pool.
 * Try to re-allocate one time if allocation fail the first time.
 *
 * @brief allocate descriptor set
 * @param descriptor pointer to descriptor set
 * @param setLayout pointer to descriptor layout
 * @param sizes collection of descriptor pool sizes
 * @return true if descriptor set allocated successfully
 */
bool DescriptorAllocator::allocate(VkDescriptorSet* descriptor, VkDescriptorSetLayout* setLayout, std::vector<VkDescriptorPoolSize> sizes) {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (_currentPool == VK_NULL_HANDLE){ // if no pool selected
        _currentPool = getPool(sizes, 0, MAX_SETS);  // todo clean up descriptorPool choice and size
        usedPools.push_back(_currentPool);
    }

    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.descriptorPool = _currentPool; // allocated from this descriptor pool
    info.descriptorSetCount = 1; // one descriptor set
    info.pSetLayouts = setLayout; // using this layout

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
        _currentPool = getPool(sizes, 0, MAX_SETS);
        info.descriptorPool = _currentPool;
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

/**
 * Recycles resources from the descriptor sets allocated from the used pools back to the descriptor pools.
 * Free descriptor sets.
 * @brief reset descriptor pools
 */
void DescriptorAllocator::resetPools() {
    for (auto p : usedPools){
        vkResetDescriptorPool(_device._logicalDevice, p, 0);
        freePools.push_back(p);
    }
    usedPools.clear();
    _currentPool = VK_NULL_HANDLE;
}

/**
 * Get a descriptor pool from which descriptor sets will be allocated.
 * Manage a list of free and used descriptor pools. This abstraction allows to allocate descriptor set easily without
 * worrying about the management of descriptor pools.

 * @brief get a free pool or return a new descriptor pool
 * @note todo - to be fixed - No comparison are made between the existing pool and the parameters provided.
 * @note todo - Free pool returned might not corresponds. Currently only one descriptor set is allocated per pool.
 * @param poolSizes how many descriptor of each type allocated from the pool (ie. uniform buffer, combined image, etc.)
 * @param flags bitmask specifying supported operations of the descriptor pool
 * @param count maximum number of sets allocated from the pool
 * @return an existing or new descriptor pool
 */
VkDescriptorPool DescriptorAllocator::getPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags, uint32_t count) {
    if (freePools.size() > 0) {
        VkDescriptorPool pool = freePools.back();
        freePools.pop_back();
        return pool;
    }

    return createPool(poolSizes, flags, count);
}

/**
 * Descriptor pool from which descriptor sets will be allocated.
 * To initialize a descriptor pool, we must specify the number of descriptor of each type needed and maximum number of sets.
 * @brief create a descriptor pool
 * @param poolSizes how many descriptor of each type allocated from the pool (ie. uniform buffer, combined image, etc.)
 * @param flags bitmask specifying supported operations of the descriptor pool
 * @param count maximum number of sets allocated from the pool
 * @return a descriptor pool
 */
VkDescriptorPool DescriptorAllocator::createPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags, uint32_t count) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.size());
    for (auto sz : poolSizes) { // counter for each descriptor type
        sizes.push_back({ sz.type, uint32_t(sz.descriptorCount * count) }); // pool size * number of sets
    }

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = flags;
    info.maxSets = count; // maximal number of set allocated
    info.poolSizeCount = static_cast<uint32_t>(sizes.size()); // how many types
    info.pPoolSizes = sizes.data(); // array of VkDescriptorPoolSize (type + count)

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(_device._logicalDevice, &info, nullptr, &descriptorPool);

    return descriptorPool;
}