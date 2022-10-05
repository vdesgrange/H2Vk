#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_camera.h"
#include <iostream>

bool Camera::on_cursor()

bool Camera::on_key(int key, int scancode, int action, int mods) {

    switch(key) {
        case GLFW_KEY_UP: forwardAction = action == GLFW_PRESS; return true;
        case GLFW_KEY_DOWN: backwardAction = action == GLFW_PRESS; return true;
        case GLFW_KEY_LEFT: leftAction = action == GLFW_PRESS; return true;
        case GLFW_KEY_RIGHT: rightAction = action == GLFW_PRESS; return true;
        case GLFW_KEY_SLASH: downAction = action == GLFW_PRESS; return true;
        case GLFW_KEY_RIGHT_SHIFT: upAction = action == GLFW_PRESS; return true;
        default: return false;
    };
}

bool Camera::update_camera(float delta) {
    const auto d = delta * speed;
    if (forwardAction) position += d * _forward;
    if (backwardAction) position -= d * _forward;
    if (rightAction) position += d * _right;
    if (leftAction) position -= d * _right;
    if (upAction) position += d * _up;
    if (downAction) position -= d * _up;

    this->update_view();
    return forwardAction || backwardAction || rightAction || leftAction || upAction || downAction;
}

const glm::mat4 Camera::get_projection_matrix() {
    return this->perspective;
}

const glm::mat4 Camera::get_view_matrix() {
    return this->view;
}

const glm::mat4 Camera::get_rotation_matrix() {
    glm::mat4 theta_rot = glm::rotate(glm::mat4{ 1 }, theta, { 0,-1,0 });
    glm::mat4 rotation = glm::rotate(glm::mat4{ theta_rot }, psi, { -1,0,0 });
    return rotation;
}

const glm::mat4 Camera::get_mesh_matrix(glm::mat4 model) {
    return this->perspective * this->view * model;
}

void Camera::update_view() {
    glm::mat4 transMat = glm::mat4(1.f);
    glm::vec3 position = this->position;
//    if (this->flipY) {
//        this->position.y *= -1;
//    }
    glm::mat4 view = glm::translate(transMat, position);
    this->view = this->flipY ? glm::inverse(view) : view;
}

void Camera::set_position(glm::vec3 position) {
    this->position = position;
    update_view();
}

void Camera::set_rotation(float theta, float psi) {
    this->theta = theta;
    this->psi = psi;
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

void Camera::inverse(bool flip) {
    this->flipY = flip;
    update_view();
}

void Camera::set_speed(float speed) {
    this->speed = speed;
}