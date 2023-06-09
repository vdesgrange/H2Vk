#include "vk_camera_system.h"
#include "ecs/components/camera.h"
#include "ecs/components/transform.h"

extern Coordinator ecs;

void CameraSystem::init() {
    for (auto& c : entities) {
        auto& camera = ecs.get_component<CameraComponent>(c);
        camera.perspective = CameraComponent::get_perspective(camera.fov, camera.aspect, camera.z_near, camera.z_far, camera.flip_y);
    }
}

void CameraSystem::update() {
    for (auto& c : entities) {
        auto& transform = ecs.get_component<TransformComponent>(c);

    }
}