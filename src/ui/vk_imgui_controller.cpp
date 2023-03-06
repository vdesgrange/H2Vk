#include "vk_imgui_controller.h"
#include "core/camera/vk_camera.h"

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
