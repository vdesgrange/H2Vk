#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include "vk_window.h"

Window::Window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _window = glfwCreateWindow(CWIDTH, CHEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

Window::~Window() {
    glfwDestroyWindow(_window);
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

double Window::get_time() {
    return glfwGetTime();
}

VkExtent2D Window::get_framebuffer_size() {
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_key) {
        return _this->on_key(key, scancode, action, mods);
    }
}

void Window::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_cursor_position) {
        return _this->on_cursor_position(xpos, ypos);
    }
}

void Window::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (_this->on_mouse_button) {
        return _this->on_mouse_button(button, action, mods);
    }
}