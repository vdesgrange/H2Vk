#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_device.h"
#include "vk_window.h"

Device::Device(Window& _window, DeletionQueue& mainDeletionQueue) {
    vkb::InstanceBuilder builder;

    //make the Vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Vulkan Application")
            .request_validation_layers(true)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger()
            .build();

    vkb::Instance vkb_inst = inst_ret.value();

    //store the instance
    _instance = vkb_inst.instance;
    //store the debug messenger
    _debug_messenger = vkb_inst.debug_messenger;

    // Get the surface of the window we opened with SDL
    if (glfwCreateWindowSurface(_instance, _window._window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    // Select a GPU
    //We want a GPU that can write to the SDL surface and supports Vulkan 1.1
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(_surface)
            .select()
            .value();

    //create the final Vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a Vulkan application
    _logicalDevice = vkbDevice.device;
    _physicalDevice = physicalDevice.physical_device;

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // Initialize memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _logicalDevice;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    mainDeletionQueue.push_function([=]() {
        vmaDestroyAllocator(_allocator);
    });
}

Device::~Device() {
    // vmaDestroyAllocator(_allocator);

//    vkDestroyDevice(_logicalDevice, nullptr);
//    vkDestroySurfaceKHR(_instance, _surface, nullptr);
//    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
//    vkDestroyInstance(_instance, nullptr);
}

VkQueue Device::get_graphics_queue() const {
    return _graphicsQueue;
}

uint32_t Device::get_graphics_queue_family() const {
    return _graphicsQueueFamily;
}