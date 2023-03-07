#pragma once

#include <GLFW/glfw3.h>
#include <functional>

#include "VkBootstrap.h"

class Window final {
public:
    static const uint32_t CWIDTH = 1200;
    static const uint32_t CHEIGHT = 800;

    GLFWwindow* _window;
    VkExtent2D _windowExtent{ CWIDTH , CHEIGHT };
    bool _framebufferResized = false;

    Window();
    ~Window();

    double get_time();
    VkExtent2D get_framebuffer_size();

    static void glfw_get_key(GLFWwindow* window);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

    std::function<void(int key, int scancode, int action, int mods)> on_key;
    std::function<void(int key, int action)> on_get_key;
    std::function<void(int button, int action, int mods)> on_mouse_button;
    std::function<void(double xpos, double ypos)> on_cursor_position;

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};