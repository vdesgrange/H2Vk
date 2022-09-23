#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_camera.h"

void Camera::update_view() {
    glm::mat4 transMat = glm::mat4(1.f);
    glm::vec3 transVec = this->position;
//    if (this->flipY) {
//        transVec.y *= -1;
//    }

    glm::mat4 translation = glm::translate(transMat, transVec);

    this->view = translation;
}

void Camera::set_position(glm::vec3 position) {
    this->position = position;
    update_view();
}

void Camera::set_perspective(float fov, float aspect, float zNear, float zFar) {
    this->fov = fov; // angle degree
    this->aspect = aspect; // width / height
    this->zNear = zNear; // near plane of the frustum
    this->zFar = zFar; // far place of the frustum

    this->perspective = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
    if (this->flipY) {
        this->perspective[1][1] *= -1;
    }
}

glm::mat4 Camera::get_mesh_matrix(glm::mat4 model) {
    return this->perspective * this->view * model;
}

float Camera::get_near_clip() {
    return this->zNear;
}

float Camera::get_far_clip() {
    return this->zFar;
}

void Camera::set_flip_y(bool flip) {
    this->flipY = flip;
    update_view();
}