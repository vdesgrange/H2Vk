/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <functional>
#include <array>
#include "core/camera/vk_camera.h"
#include "core/lighting/vk_light.h"

class Camera;

class UIController {
public:
    static std::function<float ()> get_speed(Camera& camera);
    static std::function<void (float)> set_speed(Camera& camera);
    static std::function<float ()> get_fov(Camera& camera);
    static std::function<void (float)> set_fov(Camera& camera);
    static std::function<std::array<float, 2> ()> get_z(Camera& camera);
    static std::function<void (std::array<float, 2>)> set_z(Camera& camera);
    static std::function<std::array<float, 3> ()> get_target(Camera& camera);
    static std::function<void (std::array<float, 3>)> set_target(Camera& camera);
    static std::function<int ()> get_type(Camera& camera);
    static std::function<void (int)> set_type(Camera& camera);
    static std::function<std::array<float, 3> ()> get_position(Light& light);
    static std::function<void (std::array<float, 3>)> set_position(Light& light);
    static std::function<std::array<float, 3> ()> get_color(Light& light);
    static std::function<void (std::array<float, 3>)> set_color(Light& light);
};