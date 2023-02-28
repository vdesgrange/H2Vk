#pragma once

#include <GLFW/glfw3.h>
#include <cmath>
#include <unordered_map>
#include "glm/gtc/matrix_transform.hpp"

struct GPUCameraData{
    alignas(sizeof(glm::mat4)) glm::mat4 view;
    alignas(sizeof(glm::mat4)) glm::mat4 proj;
    alignas(sizeof(glm::vec4)) glm::vec3 pos;
    alignas(sizeof(bool)) bool flip;
};

class Camera final {
public:
    enum Type {
        pov = 0,
        axis = 1,
        look_at = 2
    };
    static inline std::unordered_map<Type, const char*> AllType = {
        {pov, "Point of view"},
        {axis, "Axis"},
        {look_at, "Look at"}
    };


    bool on_key(int key, int action);
    bool on_mouse_button(int button, int action, int mods);
    bool on_cursor_position(double xpos, double ypos);

    glm::mat4 get_projection_matrix();
    glm::mat4 get_view_matrix();
    glm::vec3 get_position_vector();
    glm::mat4 get_rotation_matrix();
    glm::vec3 get_target();
    float get_aspect();
    float get_speed();
    float get_angle();
    float get_z_near();
    float get_z_far();
    bool get_flip();

    void inverse(bool flip);
    void set_type(Type type);
    void set_target(glm::vec3 target);
    void set_speed(float speed);
    void set_angle(float angle);
    void set_aspect(float aspect);
    void set_position(glm::vec3 position);
    void set_perspective(float fov, float aspect, float zNear, float zFar);

    bool update_camera(float delta);

private:
    glm::vec3 position = glm::vec3();
    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();
    glm::vec3 rotation = glm::vec3();

    Type type = Type::pov;
    bool flip_y {false};
    float fov {70.f};
    float z_near {0.01f};
    float z_far {200.f};
    float aspect {1.0f};
    float speed {1.0f};
    glm::vec3 target {glm::vec3(0.0f)};

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

    void set_rotation(glm::vec3 rotation);
    void update_view();
};
