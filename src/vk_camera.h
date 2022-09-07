#pragma once

#include <GLFW/glfw3.h>
#include <math.h>
#include "glm/gtc/matrix_transform.hpp"

struct GPUCameraData{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

class Camera final {
public:
    glm::vec3 position = glm::vec3();
    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();

    float fov; // Field of view
    float zNear;
    float zFar;
    float aspect;

    bool flipY = false;

    void set_position(glm::vec3 position);

    void set_perspective(float fov, float aspect, float zNear, float zFar);

    float get_near_clip();

    float get_far_clip();

    void set_flip_y(bool flip);

    glm::mat4 get_mesh_matrix(glm::mat4 model);

    void update_view();
};
