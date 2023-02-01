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
    enum Type { pov, axis, axis2, look_at };
    Type type = Type::axis;
    float aspect {1.0f};
    float speed {1.0f};
    float radius {1.0f};
    glm::vec3 target = glm::vec3(0.0f);

    glm::mat4 get_projection_matrix();
    glm::mat4 get_view_matrix();
    glm::vec3 get_position_vector();
    // const glm::mat4 get_rotation_matrix();

    bool on_key(int key, int action);
    bool on_mouse_button(int button, int action, int mods);
    bool on_cursor_position(double xpos, double ypos);
    bool update_camera(float delta);

    void set_position(glm::vec3 position);
    void set_rotation(glm::vec3 rotation);
    void set_perspective(float fov, float aspect, float zNear, float zFar);
    void inverse(bool flip);

    // void set_aspect(float aspect);
    // void set_speed(float speed);
private:
    glm::vec3 position = glm::vec3();
    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();
    glm::vec3 rotation = glm::vec3();

    bool flipY = false;
    float fov {70.f};
    float zNear {0.01f};
    float zFar {200.f};

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
