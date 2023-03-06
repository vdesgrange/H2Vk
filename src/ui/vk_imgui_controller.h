#pragma once

#include <functional>
#include <array>
#include "core/camera/vk_camera.h"

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
};