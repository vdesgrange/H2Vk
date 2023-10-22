/*
*  H2Vk - Performance monitoring
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_performance.h"
#include "core/vk_window.h"
#include "components/camera/vk_camera.h"


double Performance::_time {0};

/** @brief Basic FPS monitoring */
Performance::Statistics Performance::monitoring(Window* window, Camera* camera) {
    // Record delta time between calls to Render.
    const auto prevTime = _time;
    _time = window->get_time();
    const auto timeDelta = _time - prevTime;

    Statistics stats = {};
    stats.FramebufferSize = window->get_framebuffer_size();
    stats.FrameRate = static_cast<float>(1.0f / timeDelta);
    stats.coordinates[0] = camera->get_position_vector().x;
    stats.coordinates[1] = camera->get_position_vector().y;
    stats.coordinates[2] = camera->get_position_vector().z;
    return stats;
};