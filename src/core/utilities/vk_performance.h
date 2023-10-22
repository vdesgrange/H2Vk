/*
*  H2Vk - Performance monitoring
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <vulkan/vulkan.h>

class Window;
class Camera;

/**
 * @brief Performance monitoring
 * @note to be extanded with Queries mechanism
 */
class Performance final {
public:
    struct Statistics final {
        VkExtent2D FramebufferSize;
        float FrameRate;
        float coordinates[3] {0.0f, 0.0f, 0.0f};
        float rotation[3] {0.0f, 0.0f, 0.0f};
    };

    static Statistics monitoring(Window* window, Camera* camera);

private:
    static double _time;
};