#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_camera.h"
#include <iostream>

void Camera::update_view() {
    glm::vec3 position = this->position;

    if (this->type == Type::pov) {
        glm::mat4 translation =  glm::translate(glm::mat4(1.f), position);
        glm::mat4 rotation = this->get_rotation_matrix();
        this->view = translation * rotation;  // rotation around the origin of the object:
    } else if (this->type == Type::axis) {
        glm::mat4 translation =  glm::translate(glm::mat4(1.f), -1.0f * position);
        glm::mat4 rotation = this->get_rotation_matrix();
        this->view = rotation * translation; // rotation around the origin of the world
    } else {
        // this->view = glm::lookAt(this->position, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    }

    this->view = this->flipY ? glm::inverse(this->view) : this->view;
}

void Camera::set_position(glm::vec3 position) {
    this->position = position;
    update_view();
}

void Camera::set_rotation(glm::vec3 rotation) {
    this->rotation = rotation;
    if (this->rotation.x > 89.0f) this->rotation.x = 89.0f;
    if (this->rotation.x < -89.0f) this->rotation.x = -89.0f;
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
    update_view();
}

void Camera::set_aspect(float aspect) {
    this->aspect = aspect; // width / height
    update_view();
}

void Camera::inverse(bool flip) {
    this->flipY = flip;
    update_view();
}

void Camera::set_speed(float speed) {
    this->speed = speed;
}

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

bool Camera::update_camera(float delta) {
    const auto d = delta * this->speed;
    const auto radius = 1.0f;

    glm::vec3 front;
    front.x = radius * -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
    front.y = radius * sin(glm::radians(rotation.x));
    front.z = radius * cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
    front = glm::normalize(front);


    if (this->type == Type::pov) {
        const auto inverse = this->get_rotation_matrix();
        glm::vec3 right = glm::vec3(inverse * glm::vec4(1, 0, 0, 0));
        glm::vec3 up = glm::vec3(inverse * glm::vec4(0, 1, 0, 0));
        glm::vec3 forward = glm::vec3(inverse * glm::vec4(0, 0, -1, 0));

        if (forwardAction) position +=  d * forward;
        if (backwardAction) position -=  d * forward;
        if (rightAction) position +=  d * right;
        if (leftAction) position -=  d * right;
        if (upAction) position +=  d * up;
        if (downAction) position -=  d * up;


//        if (forwardAction) position -=  d * front;
//        if (backwardAction) position +=  d * front;
//        if (rightAction) position -=  d * glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
//        if (leftAction) position +=  d * glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
//        if (upAction) position +=  d * glm::normalize(glm::cross(front, glm::vec3(1.0f, 0.0f, 0.0f)));
//        if (downAction) position -=  d * glm::normalize(glm::cross(front, glm::vec3(1.0f, 0.0f, 0.0f)));

    } else if (this->type == Type::axis) {
        if (forwardAction) position -= d * front;  // to fix
        if (backwardAction) position += d * front;  // to fix
        if (rightAction) this->set_rotation(this->rotation - glm::vec3({0.0f,  10.0f * d, 0}));
        if (leftAction) this->set_rotation(this->rotation + glm::vec3({0.0f,  10.0f * d, 0}));
        if (upAction) this->set_rotation(this->rotation + glm::vec3({10.0f * d, 0.0f, 0.0f }));
        if (downAction) this->set_rotation(this->rotation - glm::vec3({10.0f * d, 0.0f, 0.0f }));
    } else {  // Type::lookat

    }

    this->update_view();
    return forwardAction || backwardAction || rightAction || leftAction || upAction || downAction;
}

bool Camera::on_cursor_position(double xpos, double ypos) {
    const auto deltaY = static_cast<float>(xpos - mousePositionX);
    const auto deltaX = static_cast<float>(ypos - mousePositionY);

    if (mouseLeft) {
        this->set_rotation(this->rotation + 0.1f * glm::vec3({deltaX, deltaY, 0}));
    }

    mousePositionX = xpos;
    mousePositionY = ypos;

    return mouseLeft;
}

bool Camera::on_mouse_button(int button, int action, int mods) {
    switch(button) {
        case GLFW_MOUSE_BUTTON_LEFT: mouseLeft = action == GLFW_PRESS; return mouseLeft;
        case GLFW_MOUSE_BUTTON_RIGHT: mouseRight = action == GLFW_PRESS; return mouseRight;
    };

    return false;
}

const glm::mat4 Camera::get_projection_matrix() {
    return this->perspective;
}

const glm::mat4 Camera::get_view_matrix() {
    return this->view;
}

const glm::vec3 Camera::get_position_vector() {
    return this->position;
}

const glm::mat4 Camera::get_rotation_matrix() {
    glm::mat4 rot = glm::mat4{ 1.0f };
    rot = glm::rotate(rot, glm::radians(this->rotation.x), {1, 0, 0});
    rot = glm::rotate(rot, glm::radians(this->rotation.y), {0, 1, 0});
    rot = glm::rotate(rot, glm::radians(this->rotation.z), {0, 0, 1});
    return rot;
}
