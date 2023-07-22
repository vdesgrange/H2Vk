/*
*  H2Vk - Window class
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_window.h"

/**
 *  Default Window constructor
 */
Window::Window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _window = glfwCreateWindow(CWIDTH, CHEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    // glfwSetKeyCallback(_window, glfw_key_callback); // avoid call back for continue action like key press
    glfwSetCursorPosCallback(_window, glfw_cursor_position_callback);
    glfwSetMouseButtonCallback(_window, glfw_mouse_button_callback);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}


Window::~Window() {
    glfwDestroyWindow(_window);
}

/**
 * Framebuffer resized callback function
 * @param window Encapsulates window and context
 * @param width
 * @param height
 */
void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->_framebufferResized = true;
}

double Window::get_time() {
    return glfwGetTime();
}

/**
 * Return window extent
 * @return VkExtent2D
 */
VkExtent2D Window::get_framebuffer_size() {
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

/**
 * Key callback called when the cursor is moved.
 * @note Unused: We should avoid call back for continue action like key press
 */
void Window::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_key) {
        return _this->on_key(key, scancode, action, mods);
    }
}

/**
 * Cursor position callback called when the cursor is moved.
 * Use the position in screen coordinates, relative to the upper-left corner of the content area of the window.
 * @param window
 * @param xpos x axis position
 * @param ypos y axis position
 */
void Window::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_cursor_position) {
        return _this->on_cursor_position(xpos, ypos);
    }
}

/**
 * Callback function invoked when a mouse button is pressed or released.
 * @param window Encapsulates GLFW window and context
 */
void Window::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_mouse_button) {
        return _this->on_mouse_button(button, action, mods);
    }
}

/**
 * Invoke lambda when a key is pressed or released.
 * Call with key's id pressed and key's action id.
 * @param window Encapsulates GLFW window and context
 */
void Window::glfw_get_key(GLFWwindow* window) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_get_key) {
        _this->on_get_key(GLFW_KEY_UP, glfwGetKey(window, GLFW_KEY_UP));
        _this->on_get_key(GLFW_KEY_DOWN, glfwGetKey(window, GLFW_KEY_DOWN));
        _this->on_get_key(GLFW_KEY_RIGHT, glfwGetKey(window, GLFW_KEY_RIGHT));
        _this->on_get_key(GLFW_KEY_LEFT, glfwGetKey(window, GLFW_KEY_LEFT));
        _this->on_get_key(GLFW_KEY_SLASH, glfwGetKey(window, GLFW_KEY_SLASH));
        _this->on_get_key(GLFW_KEY_RIGHT_SHIFT, glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));
    }
}