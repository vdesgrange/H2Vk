#pragma once

#include <GLFW/glfw3.h>
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

class Device final {

    class Window;

public:
    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _physicalDevice; // GPU chosen as the default device
    VkDevice _logicalDevice; // Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window surface

    VmaAllocator _allocator;

    Device(GLFWwindow* _window);
    ~Device();

    VkQueue get_graphics_queue() const;
    uint32_t get_graphics_queue_family() const;

private:
    GLFWwindow* _window;

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
};