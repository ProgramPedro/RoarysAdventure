// Implements the per-frame input snapshot: last frame's state is copied to
// 'previous' so up-to-down changes can be spotted, then the current keys, mouse
// button and cursor position are read from GLFW. The cursor is scaled from window
// coordinates into framebuffer pixels so it lines up on high-DPI displays.

#include "core/Input.h"

#include <GLFW/glfw3.h>

#include <algorithm>

void Input::update(GLFWwindow* window) {
    previous_ = current_;
    mousePrevious_ = mouseCurrent_;

    for (int key = 0; key < kKeyCount; ++key) {
        current_[static_cast<std::size_t>(key)] =
            (glfwGetKey(window, key) == GLFW_PRESS);
    }

    mouseCurrent_ =
        (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    int windowWidth = 0, windowHeight = 0;
    int bufferWidth = 0, bufferHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);

    const float scaleX = (windowWidth > 0)
        ? static_cast<float>(bufferWidth) / static_cast<float>(windowWidth) : 1.0f;
    const float scaleY = (windowHeight > 0)
        ? static_cast<float>(bufferHeight) / static_cast<float>(windowHeight) : 1.0f;

    mousePosition_.x = static_cast<float>(cursorX) * scaleX;
    mousePosition_.y = static_cast<float>(cursorY) * scaleY;
}

bool Input::isDown(int glfwKey) const {
    if (glfwKey < 0 || glfwKey >= kKeyCount) return false;
    return current_[static_cast<std::size_t>(glfwKey)];
}

bool Input::wasPressed(int glfwKey) const {
    if (glfwKey < 0 || glfwKey >= kKeyCount) return false;
    const auto index = static_cast<std::size_t>(glfwKey);
    return current_[index] && !previous_[index];
}

bool Input::isMouseDown() const {
    return mouseCurrent_;
}

bool Input::wasMouseClicked() const {
    return mouseCurrent_ && !mousePrevious_;
}
