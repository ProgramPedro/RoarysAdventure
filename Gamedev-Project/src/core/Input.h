#pragma once

// One frame's snapshot of keyboard and mouse state, taken once per frame after
// events are polled. It keeps the previous frame as well, so wasPressed() can
// report the single frame a key went down - without that, one tap of a menu key
// would scroll through every entry at the frame rate.

#include <glm/vec2.hpp>
#include <array>

struct GLFWwindow;

class Input {
public:
    void update(GLFWwindow* window);

    bool isDown(int glfwKey) const;

    bool wasPressed(int glfwKey) const;

    const glm::vec2& mousePosition() const { return mousePosition_; }

    bool isMouseDown() const;
    bool wasMouseClicked() const;

private:
    static constexpr int kKeyCount = 349;

    std::array<bool, kKeyCount> current_{};
    std::array<bool, kKeyCount> previous_{};

    bool mouseCurrent_ = false;
    bool mousePrevious_ = false;

    glm::vec2 mousePosition_{0.0f, 0.0f};
};
