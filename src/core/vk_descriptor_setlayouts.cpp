#include "vk_descriptor_setlayouts.h"
#include "vk_initializers.h"
#include "vk_device.h"

DescriptorSetLayouts::DescriptorSetLayouts(const Device& device,  std::vector<VkDescriptorSetLayoutBinding> bindings) : _device(device) {
    VkDescriptorSetLayoutCreateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = nullptr;
    setInfo.flags = 0;
    setInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    setInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(device._logicalDevice, &setInfo, nullptr, &_globalSetLayout);
}