/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include "vk_camera.h"
#include "core/vk_device.h"
#include "core/utilities/vk_global.h"
#include <cmath>

bool Camera::on_key(int key, int action) {
    switch(key) {
        case GLFW_KEY_UP: forwardAction = action == GLFW_PRESS; return forwardAction;
        case GLFW_KEY_DOWN: backwardAction = action == GLFW_PRESS; return backwardAction;
        case GLFW_KEY_LEFT: leftAction = action == GLFW_PRESS; return leftAction;
        case GLFW_KEY_RIGHT: rightAction = action == GLFW_PRESS; return rightAction;
        case GLFW_KEY_SLASH: downAction = action == GLFW_PRESS; return downAction;
        case GLFW_KEY_RIGHT_SHIFT: upAction = action == GLFW_PRESS; return upAction;
        default: return false;
    };
}

bool Camera::on_mouse_button(int button, int action, int mods) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: mouseLeft = action == GLFW_PRESS; return mouseLeft;
        case GLFW_MOUSE_BUTTON_RIGHT: mouseRight = action == GLFW_PRESS; return mouseRight;
        default: return false;
    };
}

bool Camera::on_cursor_position(double xpos, double ypos) {
    const auto deltaY = this->speed * static_cast<float>(xpos - mousePositionX);
    const auto deltaX = this->speed * static_cast<float>(ypos - mousePositionY);

    if (mouseLeft) {
        if (this->type == Type::look_at) {
            glm::vec3 pos = this->position;
            float radius = sqrt(pow(pos.x, 2) + pow(pos.y, 2) + pow(pos.z, 2));
            float theta = atan2(pos.x, pos.z) +  0.001f * deltaY;
            float phi = acos(pos.y / radius) - 0.001f * deltaX;

            phi = fmax(fmin(phi, float(M_PI - 0.1f)), 0.1f);

            this->set_position(radius * glm::vec3(sin(phi) * sin(theta), cos(phi), cos(theta) * sin(phi)));
        } else {
            this->set_rotation(this->rotation + 0.1f * glm::vec3({deltaX, deltaY, 0}));
        }
    }
    mousePositionX = xpos;
    mousePositionY = ypos;

    return mouseLeft;
}

glm::mat4 Camera::get_projection_matrix() {
    return this->perspective;
}

glm::mat4 Camera::get_view_matrix() {
    return this->view;
}

glm::vec3 Camera::get_position_vector() {
    return this->position;
}

glm::vec3 Camera::get_rotation_vector() {
    return this->rotation;
}

glm::mat4 Camera::get_rotation_matrix() {
    glm::mat4 rot = glm::mat4{ 1.0f };
    rot = glm::rotate(rot, glm::radians(this->rotation.x), {1.0f, 0.0f, 0.0f});
    rot = glm::rotate(rot, glm::radians(this->rotation.y), {0.0f, 1.0f, 0.0f});
    rot = glm::rotate(rot, glm::radians(this->rotation.z), {0.0f, 0.0f, 1.0f});
    return rot;
}

glm::vec3 Camera::get_target() {
    return this->target;
}

bool Camera::get_flip() {
    return this->flip_y;
}

float Camera::get_aspect() {
    return this->aspect;
}

float Camera::get_speed() {
    return this->speed;
}

float Camera::get_angle() {
    return this->fov;
}

float Camera::get_z_near() {
    return this->z_near;
}

float Camera::get_z_far() {
    return this->z_far;
}

Camera::Type Camera::get_type() {
    return this->type;
}

void Camera::inverse(bool flip) {
    this->flip_y = flip;
    update_view();
}

void Camera::set_type(Camera::Type type) {
    this->type = type;
    update_view();
}

void Camera::set_target(glm::vec3 target) {
    this->target = target;
    update_view();
}

void Camera::set_speed(float speed) {
    this->speed = speed;
}

void Camera::set_aspect(float aspect) {
    this->aspect = aspect;
}

void Camera::set_position(glm::vec3 position) {
    this->position = position;
    update_view();
}

void Camera::set_angle(float a) {
    this->fov = a;
}

void Camera::set_perspective(float fov, float aspect, float zNear, float zFar) {
    this->fov = fov; // angle degree
    this->aspect = aspect; // width / height
    this->z_near = zNear; // near plane of the frustum
    this->z_far = zFar; // far place of the frustum

    this->perspective = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
    if (this->flip_y) {
        this->perspective[1][1] *= -1;
    }
    update_view();
}

void Camera::set_rotation(glm::vec3 rotation) {
    this->rotation = rotation;
    if (this->rotation.x > 89.0f) this->rotation.x = 89.0f;
    if (this->rotation.x < -89.0f) this->rotation.x = -89.0f;
    update_view();
}

bool Camera::update_camera(float delta) {
    const auto d = delta * this->speed;

    if (this->type == Type::pov) {
        glm::vec3 front;
        front.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
        front.y = sin(glm::radians(rotation.x));
        front.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
        front = glm::normalize(front);

        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        if (forwardAction) position -=  d * front;
        if (backwardAction) position +=  d * front;
        if (rightAction) position -=  d * glm::normalize(glm::cross(front, up));
        if (leftAction) position +=  d * glm::normalize(glm::cross(front, up));
        if (upAction) position +=  d * up;
        if (downAction) position -=  d * up;

    } else {
        glm::vec3 pos = this->position;
        float radius = sqrt(pow(pos.x, 2) + pow(pos.y, 2) + pow(pos.z, 2));
        float theta = atan2(pos.x, pos.z);
        float phi = acos(pos.y / radius);

        if (forwardAction) radius = fmax(radius - d, 0.1f);
        if (backwardAction) radius += d;
        if (rightAction) theta += d;
        if (leftAction) theta -= d;
        if (upAction) phi = fmax(phi - d, 0.1f);
        if (downAction) phi = fmin(phi + d, float(M_PI - 0.1f));

        this->set_position(radius * glm::vec3(sin(phi) * sin(theta), cos(phi), cos(theta) * sin(phi)));
    }

    this->update_view();
    return forwardAction || backwardAction || rightAction || leftAction || upAction || downAction;
}

void Camera::update_view() {
    if (this->type == Type::look_at) {
        this->view = glm::lookAt(this->position, this->target, glm::vec3(0.0, 1.0, 0.0));
    } else {
        // To method, the result should be the same matrix
        // glm::vec3 front;
        // front.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
        // front.y = sin(glm::radians(rotation.x));
        // front.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
        // front = glm::normalize(front);
        // this->view = glm::lookAt(this->position, this->position + front, glm::vec3(0.0, 1.0, 0.0));

        glm::mat4 transM = glm::translate(glm::mat4(1.f), glm::vec3(-1.0) * this->position);
        glm::mat4 rotM = this->get_rotation_matrix();
        this->view = rotM * transM;  // Rotation around camera's origin. Swap for world origin.
    }
}

void Camera::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        Buffer::create_buffer(device, &g_frames[i].cameraBuffer, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
}

GPUCameraData Camera::gpu_format() {
    GPUCameraData camData{};
    camData.proj = this->get_projection_matrix();
    camData.view = this->get_view_matrix();
    camData.pos = this->get_position_vector();
    camData.flip = this->get_flip();

    return camData;
}