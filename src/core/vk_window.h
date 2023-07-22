/*
*  H2Vk - Window class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <GLFW/glfw3.h>
#include <functional>

#include "VkBootstrap.h"

class Window final {
public:
    /** @brief Default surface width */
    static const uint32_t CWIDTH = 1200;
    /** @brief Default surface height */
    static const uint32_t CHEIGHT = 800;

    /** @brief Encapsulates window and context */
    GLFWwindow* _window;
    /** @brief Specify a two-dimensional extent */
    VkExtent2D _windowExtent{ CWIDTH , CHEIGHT };
    /** @brief Specify if framebuffer must be resized */
    bool _framebufferResized = false;

    Window();
    ~Window();

    double get_time();
    VkExtent2D get_framebuffer_size();

    static void glfw_get_key(GLFWwindow* window);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

    /** @brief function wrapper used for continuous key callback. Not used anymore */
    std::function<void(int key, int scancode, int action, int mods)> on_key;
    /** @brief function wrapper used for on key action. Invoke lambda define in main engine */
    std::function<void(int key, int action)> on_get_key;
    /** @brief function wrapper used for on mouse button action. Invoke lambda define in main engine */
    std::function<void(int button, int action, int mods)> on_mouse_button;
    /** @brief function wrapper used for on mouse button action. Invoke lambda define in main engine */
    std::function<void(double xpos, double ypos)> on_cursor_position;

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};