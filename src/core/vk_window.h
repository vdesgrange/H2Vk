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

private:
    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};