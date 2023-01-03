#pragma once

#include <GLFW/glfw3.h>
#include <cmath>
#include "glm/gtc/matrix_transform.hpp"

struct GPUCameraData{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 pos;
};

class Camera final {
public:
    enum Type { pov, axis, look_at };
    Type type = Type::axis;

    const glm::mat4 get_projection_matrix();
    const glm::mat4 get_view_matrix();
    const glm::vec3 get_position_vector();
    const glm::mat4 get_rotation_matrix();

    bool on_key(int key, int scancode, int action, int mods);
    bool on_key(int key, int action);
    bool on_cursor_position(double xpos, double ypos);
    bool on_mouse_button(int button, int action, int mods);
    bool update_camera(float delta);

    void set_position(glm::vec3 position);
    void set_rotation(glm::vec3 rotation);
    void set_rotation(glm::mat4 rotation);
    void set_perspective(float fov, float aspect, float zNear, float zFar);
    void set_speed(float speed);
    void inverse(bool flip);

private:
    glm::vec3 position = glm::vec3();
    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();
    glm::vec3 rotation = glm::vec3();
    glm::vec3 target = glm::vec3(0.0f);

    float speed {1.0f};
    float fov {70.f};
    float zNear {0.01f};
    float zFar {200.f};
    float aspect {1};
    bool flipY = false;

    bool leftAction {false};
    bool rightAction {false};
    bool upAction {false};
    bool downAction {false};
    bool forwardAction {false};
    bool backwardAction {false};
    bool mouseLeft{false};
    bool mouseRight {false};
    float mousePositionX{0};
    float mousePositionY{0};

    void update_view();

//    void direction();
//    glm::vec3 _right{ 1, 0, 0 };
//    glm::vec3 _up{ 0, 1, 0};
//    glm::vec3 _forward{ 0, 0, -1};

//    glm::vec3 _rotation{0, 0, 0};
};
