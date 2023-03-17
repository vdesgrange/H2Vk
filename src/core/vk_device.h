#pragma once

#include "vk_mem_alloc.h"

class Window;

class Device final {

public:
    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _physicalDevice; // GPU chosen as the default device
    VkDevice _logicalDevice; // Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window surface

    VkPhysicalDeviceProperties _gpuProperties; // request gpu information

    VmaAllocator _allocator;

    Device(Window& _window);
    ~Device();

    VkQueue get_graphics_queue() const;
    uint32_t get_graphics_queue_family() const;

private:
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
};