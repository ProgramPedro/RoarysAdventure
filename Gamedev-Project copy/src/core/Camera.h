#pragma once
//=============================================================================
// Camera - stores where we are looking from and hands out the view and
// projection matrices. Deliberately simple: a look-at camera with no
// first-person controls, which is all the menu needs.
//
// Later levels can either reuse this (set position/target every frame to
// follow Roary) or add their own camera class.
//=============================================================================

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    Camera() = default;
    Camera(const glm::vec3& position, const glm::vec3& target);

    void setPosition(const glm::vec3& position) { position_ = position; }
    void setTarget(const glm::vec3& target)     { target_ = target; }
    void setFovDegrees(float fov)               { fovDegrees_ = fov; }

    const glm::vec3& position() const { return position_; }
    const glm::vec3& target() const   { return target_; }

    glm::mat4 view() const;

    // Perspective projection using the live framebuffer size for aspect ratio.
    glm::mat4 perspective(int framebufferWidth, int framebufferHeight) const;

private:
    glm::vec3 position_{0.0f, 0.0f, 10.0f};
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};
    float fovDegrees_ = 45.0f;
};

//-----------------------------------------------------------------------------
// Projects a world-space point into screen pixel coordinates, with the origin
// at the TOP-LEFT of the window (the same convention TextRenderer uses).
//
// Returns false if the point is behind the camera, in which case the output is
// not usable. Used by the menu to place floor labels and to hit-test the mouse
// against each floor.
//-----------------------------------------------------------------------------
bool worldToScreen(const glm::vec3& worldPos,
                   const glm::mat4& viewProjection,
                   int framebufferWidth, int framebufferHeight,
                   glm::vec3& outScreen);
