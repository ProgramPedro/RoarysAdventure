#pragma once

// A look-at camera that stores a position and a target and hands out the view
// and projection matrices. Its up vector can be set freely, which is what lets
// the tunnel level roll the player's sense of down. Also declares worldToScreen,
// used for placing labels and hit-testing the mouse.

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    Camera() = default;
    Camera(const glm::vec3& position, const glm::vec3& target);

    void setPosition(const glm::vec3& position) { position_ = position; }
    void setTarget(const glm::vec3& target)     { target_ = target; }
    void setFovDegrees(float fov)               { fovDegrees_ = fov; }

    void setUp(const glm::vec3& up)              { up_ = up; }

    const glm::vec3& position() const { return position_; }
    const glm::vec3& target() const   { return target_; }

    glm::mat4 view() const;

    glm::mat4 perspective(int framebufferWidth, int framebufferHeight) const;

private:
    glm::vec3 position_{0.0f, 0.0f, 10.0f};
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};
    float fovDegrees_ = 45.0f;
};

bool worldToScreen(const glm::vec3& worldPos,
                   const glm::mat4& viewProjection,
                   int framebufferWidth, int framebufferHeight,
                   glm::vec3& outScreen);
