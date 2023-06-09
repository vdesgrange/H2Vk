#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct CameraComponent {
    float fov = 70.f;
    float aspect = 1.0f;
    float z_near = 0.01f;
    float z_far = 200.f;
    bool flip_y = false;
    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();

    static glm::mat4 get_perspective(float fov, float aspect, float zNear, float zFar, bool flipY) {
        glm::mat4 perspective = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
        perspective[1][1] *= flipY ? -1 : 1;
        return perspective;
    }

};


