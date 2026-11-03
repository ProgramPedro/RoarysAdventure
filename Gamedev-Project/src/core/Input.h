#pragma once
//=============================================================================
// Input - one frame's worth of keyboard and mouse state.
//
// The important part is wasPressed(): it is true only on the single frame a key
// goes down. Menus need that. If we only had "is the key held", one tap of the
// Down arrow would scroll through every floor at 60 steps per second.
//=============================================================================

#include <glm/vec2.hpp>
#include <array>

struct GLFWwindow;

class Input {
public:
    // Call once per frame, AFTER glfwPollEvents(). Copies this frame's state
    // over last frame's so the edge detection below works.
    void update(GLFWwindow* window);

    // True for as long as the key is held down.
    bool isDown(int glfwKey) const;

    // True only on the frame the key transitioned from up to down.
    bool wasPressed(int glfwKey) const;

    // Mouse position in FRAMEBUFFER pixels, origin at the top-left. Scaled for
    // Retina/high-DPI displays, where the framebuffer is bigger than the window.
    const glm::vec2& mousePosition() const { return mousePosition_; }

    bool isMouseDown() const;       // left button held
    bool wasMouseClicked() const;   // left button pressed this frame

private:
    // GLFW key codes run from 32 (space) up to GLFW_KEY_LAST (348). Tracking
    // the whole range is only a few hundred bytes and keeps the code simple.
    static constexpr int kKeyCount = 349;

    std::array<bool, kKeyCount> current_{};
    std::array<bool, kKeyCount> previous_{};

    bool mouseCurrent_ = false;
    bool mousePrevious_ = false;

    glm::vec2 mousePosition_{0.0f, 0.0f};
};
