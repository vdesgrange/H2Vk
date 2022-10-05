#pragma once

#include <GLFW/glfw3.h>
#include "VkBootstrap.h"

const uint32_t CWIDTH = 800;
const uint32_t CHEIGHT = 600;

class Window final {
public:
    GLFWwindow* _window;
    VkExtent2D _windowExtent{ CWIDTH , CHEIGHT };

    Window();
    ~Window();

    double get_time();
    VkExtent2D get_framebuffer_size();

    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    std::function<void(int key, int scancode, int action, int mods)> on_key;

private:
    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};