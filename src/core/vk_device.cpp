/*
*  H2Vk - Device class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_device.h"
#include "vk_window.h"
#include <iostream>

/**
 * Default Device constructor
 *
 * @param window Surface window to be used
 */
Device::Device(Window& window) {
    vkb::InstanceBuilder builder;

    //make the Vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("H2Vk")
            .request_validation_layers(true)
            .require_api_version(1, 2, 0)
            .use_default_debug_messenger()
            .add_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) // for shader debugging
            .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
            .build();

    vkb::Instance vkb_inst = inst_ret.value();

    //store the instance
    _instance = vkb_inst.instance;
    //store the debug messenger
    _debug_messenger = vkb_inst.debug_messenger;

    // Get the surface of the window we opened with SDL
    if (glfwCreateWindowSurface(_instance, window._window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    // Select a GPU
    //We want a GPU that can write to the GLFW surface and supports Vulkan 1.1
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    VkPhysicalDeviceFeatures required_features {};
    required_features.depthClamp = VK_TRUE;
    required_features.samplerAnisotropy = VK_TRUE;

    // Enable Vulkan features
    VkPhysicalDeviceVulkan11Features features11 = {};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.shaderDrawParameters = VK_TRUE;
    features11.pNext = nullptr;

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.hostQueryReset = VK_TRUE;
    features12.pNext = nullptr;

    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 2)
            .set_surface(_surface)
            .allow_any_gpu_device_type(false)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete) // NO-INTEL = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
//            .add_desired_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)
//            .add_desired_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
            .add_desired_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME)
            .set_required_features(required_features)
            .set_required_features_11(features11) // Enable selected Vulkan 1.1 features
            .set_required_features_12(features12) // Enable selected Vulkan 1.2 features
            .select(vkb::DeviceSelectionMode::partially_and_fully_suitable)
            .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a Vulkan application
    _logicalDevice = vkbDevice.device;
    _physicalDevice = physicalDevice.physical_device;
    _gpuProperties = physicalDevice.properties;

    // Note : Uncomment to check which Vulkan features (12 or 11 and 13) are enabled.
//    _gpuFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
//    _gpuFeatures2.pNext = &features12;
//    vkGetPhysicalDeviceFeatures2(_physicalDevice, &_gpuFeatures2);

    std::cout << "GPU properties : " << _gpuProperties.deviceType << " : " << _gpuProperties.deviceName << std::endl;

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // Initialize memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _logicalDevice;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

Device::~Device() {
//    vmaDestroyAllocator(_allocator);
//    vkDestroyDevice(_logicalDevice, nullptr);
//    vkDestroySurfaceKHR(_instance, _surface, nullptr);
//    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
//    vkDestroyInstance(_instance, nullptr);
}
/**
 * Get graphics queue pointer.
 * Assume to wrap both present and graphics operation. Single graphics queue for basic GPU.
 * @return
 */
VkQueue Device::get_graphics_queue() const {
    return _graphicsQueue;
}

/**
 * Get graphics queue index
 * @return
 */
uint32_t Device::get_graphics_queue_family() const {
    return _graphicsQueueFamily;
}