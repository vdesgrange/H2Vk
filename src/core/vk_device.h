/*
*  H2Vk - Device class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_mem_alloc.h"

class Window;

/**
 * Class wrapping Vulkan physical and logical device representations
 */
class Device final {

public:
    /** @brief Initializes the Vulkan library and allows the application to pass information about itself */
    VkInstance _instance;
    /** @brief Vulkan debug output handle */
    VkDebugUtilsMessengerEXT _debug_messenger; //
    /** @brief Physical device representation. GPU chosen as default device */
    VkPhysicalDevice _physicalDevice;
    /** @brief Logical device representation */
    VkDevice _logicalDevice;
    /** @brief Native platform window surface abstraction */
    VkSurfaceKHR _surface;
    /** @brief GPU information */
    VkPhysicalDeviceProperties _gpuProperties;
    /** @brief GPU features 2 */
    VkPhysicalDeviceFeatures2 _gpuFeatures2;
    /** @brief Represent memory assigned to a buffer */
    VmaAllocator _allocator;

    explicit Device(Window& _window);
    ~Device();

    VkQueue get_graphics_queue() const;
    uint32_t get_graphics_queue_family() const;

private:
    /** @brief Device graphics queue where command buffers are submitted to */
    VkQueue _graphicsQueue;

    uint32_t _graphicsQueueFamily;
};