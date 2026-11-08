// Implements the camera: builds the view matrix with glm::lookAt and the
// projection from the current aspect ratio, guarding against a zero height when
// the window is minimized. Also implements worldToScreen, which projects a world
// point into framebuffer pixels and reports whether it is in front of the camera.

#include "core/Camera.h"

#include "core/Config.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include <algorithm>

Camera::Camera(const glm::vec3& position, const glm::vec3& target)
    : position_(position), target_(target) {}

glm::mat4 Camera::view() const {
    return glm::lookAt(position_, target_, up_);
}

glm::mat4 Camera::perspective(int framebufferWidth, int framebufferHeight) const {
    const float safeHeight = static_cast<float>(std::max(framebufferHeight, 1));
    const float aspect = static_cast<float>(std::max(framebufferWidth, 1)) / safeHeight;
    return glm::perspective(glm::radians(fovDegrees_), aspect,
                            cfg::kCameraNear, cfg::kCameraFar);
}

bool worldToScreen(const glm::vec3& worldPos,
                   const glm::mat4& viewProjection,
                   int framebufferWidth, int framebufferHeight,
                   glm::vec3& outScreen) {
    const glm::vec4 clip = viewProjection * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0001f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    outScreen.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(framebufferWidth);
    outScreen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(framebufferHeight);
    outScreen.z = ndc.z;
    return true;
}
