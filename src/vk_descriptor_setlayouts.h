#pragma once

#include <vector>

#include "VkBootstrap.h"

class Device;

class DescriptorSetLayouts final {
public:
    VkDescriptorSetLayout _globalSetLayout;

    DescriptorSetLayouts(const Device& device, std::vector<VkDescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayouts();

private:
    const class Device& _device;
};