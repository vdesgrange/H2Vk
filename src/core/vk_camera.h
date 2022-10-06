#pragma once

#include <GLFW/glfw3.h>
#include <cmath>
#include "glm/gtc/matrix_transform.hpp"

struct GPUCameraData{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

class Camera final {
public:
    const glm::mat4 get_projection_matrix();
    const glm::mat4 get_view_matrix();
    const glm::mat4 get_rotation_matrix();
    const glm::mat4 get_mesh_matrix(glm::mat4 model);

    bool on_key(int key, int scancode, int action, int mods);
    bool on_cursor_position(double xpos, double ypos);
    bool on_mouse_button(int button, int action, int mods);
    bool update_camera(float delta);

    void set_position(glm::vec3 position);
    void set_rotation(float theta, float psi);
    void set_perspective(float fov, float aspect, float zNear, float zFar);
    void set_speed(float speed);
    void inverse(bool flip);

private:
    glm::vec3 position = glm::vec3();
    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();

    float speed {1};
    float fov {70.f}; // Field of view
    float zNear {0.01f};
    float zFar {200.f};
    float aspect {1};
    float psi {0}; // up-down
    float theta {0}; // right-left
    bool flipY = false;

    glm::vec3 _right{ 1, 0, 0 };
    glm::vec3 _up{ 0, 1, 0};
    glm::vec3 _forward{ 0, 0, -1};
    glm::vec2 _rotation{0, 0};

    bool leftAction {false};
    bool rightAction {false};
    bool upAction {false};
    bool downAction {false};
    bool forwardAction {false};
    bool backwardAction {false};
    bool mouseRight {false};
    float mousePositionX{0};
    float mousePositionY{0};

    void update_view();
};
