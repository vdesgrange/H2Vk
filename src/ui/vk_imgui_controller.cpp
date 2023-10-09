/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_imgui_controller.h"
#include "components/camera/vk_camera.h"

std::function<int ()> UIController::get_type(Camera& camera) {
    return [&]() { return static_cast<int>(camera.get_type()); };
}

std::function<void (int)> UIController::set_type(Camera& camera) {
    return [&](int type) { camera.set_type(static_cast<Camera::Type>(type)); };
}

std::function<float ()> UIController::get_speed(Camera& camera) {
    return [&]() { return camera.get_speed(); };
}

std::function<void (float)> UIController::set_speed(Camera& camera) {
    return [&](float s) { camera.set_speed(s); };
}

std::function<float ()> UIController::get_fov(Camera& camera) {
    return [&]() { return camera.get_angle(); };
}

std::function<void (float)> UIController::set_fov(Camera& camera) {
    return [&](float fov) {
        camera.set_perspective(fov, camera.get_aspect(), camera.get_z_near(), camera.get_z_far());
    };
}

std::function<std::array<float, 2> ()> UIController::get_z(Camera& camera) {
    return [&]() {
        return std::array<float, 2>({camera.get_z_near(), camera.get_z_far()});
    };
}

std::function<void (std::array<float, 2>)> UIController::set_z(Camera& camera) {
    return [&](std::array<float, 2> z) {
        camera.set_perspective(camera.get_angle(), camera.get_aspect(), z[0], z[1]);
    };
}

std::function<std::array<float, 3> ()> UIController::get_target(Camera& camera) {
    return [&]() {
        glm::vec3 t = camera.get_target();
        return std::array<float, 3>({t.x, t.y, t.z});
    };
}

std::function<void (std::array<float, 3>)> UIController::set_target(Camera& camera) {
    return [&](std::array<float, 3> z) {
        camera.set_target(glm::vec3(z[0], z[1], z[2]));
    };
}

std::function<std::array<float, 3> ()> UIController::get_position(Light& light) {
    return [&]() {
        glm::vec3 p = light.get_position();
        return std::array<float, 3>({p.x, p.y, p.z});
    };
}

std::function<void (std::array<float, 3>)> UIController::set_position(Light& light) {
    return [&](std::array<float, 3> p) {
        light.set_position(glm::vec4(p[0], p[1], p[2], 0.0f));
    };
}

std::function<std::array<float, 3> ()> UIController::get_rotation(Light& light) {
    return [&]() {
        glm::vec3 p = light.get_rotation();
        return std::array<float, 3>({p.x, p.y, p.z});
    };
}

std::function<void (std::array<float, 3>)> UIController::set_rotation(Light& light) {
    return [&](std::array<float, 3> p) {
        light.set_rotation(glm::vec4(p[0], p[1], p[2], 0.0f));
    };
}

std::function<std::array<float, 3> ()> UIController::get_color(Light& light) {
    return [&]() {
        glm::vec3 c = light.get_color();
        return std::array<float, 3>({c.x, c.y, c.z});
    };
}

std::function<void (std::array<float, 3>)> UIController::set_color(Light& light) {
    return [&](std::array<float, 3> c) {
        light.set_position(glm::vec4(c[0], c[1], c[2], 1.0f));
    };
}

std::function<float ()> UIController::get_lambda(CascadedShadow& csm) {
    return [&]() { return csm._lb; };
}

std::function<void (float)> UIController::set_lambda(CascadedShadow& csm) {
    return [&](float lb) {
        csm._lb = lb;
    };
}
