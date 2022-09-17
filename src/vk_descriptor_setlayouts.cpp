#include "vk_descriptor_setlayouts.h"
#include "vk_initializers.h"
#include "vk_device.h"

DescriptorSetLayouts::DescriptorSetLayouts(const Device& device,  std::vector<VkDescriptorSetLayoutBinding> bindings) : _device(device) {
//    VkDescriptorSetLayoutBinding camBufferBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
//    VkDescriptorSetLayoutBinding sceneBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
//    VkDescriptorSetLayoutBinding objectsBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
//    VkDescriptorSetLayoutBinding textureBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
//    VkDescriptorSetLayoutBinding bindings[] = { camBufferBinding, sceneBinding };

    // --- Info
    VkDescriptorSetLayoutCreateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = nullptr;
    setInfo.flags = 0;
    setInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    setInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(device._logicalDevice, &setInfo, nullptr, &_globalSetLayout);

//    VkDescriptorSetLayoutCreateInfo setInfo2{};
//    setInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    setInfo2.pNext = nullptr;
//    setInfo2.flags = 0;
//    setInfo2.bindingCount = 1;
//    setInfo2.pBindings = &objectsBinding;
//
//    vkCreateDescriptorSetLayout(_device->_logicalDevice, &setInfo2, nullptr, &_objectSetLayout);
//
//    VkDescriptorSetLayoutCreateInfo setInfo3{};
//    setInfo3.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    setInfo3.pNext = nullptr;
//    setInfo3.flags = 0;
//    setInfo3.bindingCount = 1;
//    setInfo3.pBindings = &textureBinding;
//
//    vkCreateDescriptorSetLayout(_device->_logicalDevice, &setInfo3, nullptr, &_singleTextureSetLayout);

}