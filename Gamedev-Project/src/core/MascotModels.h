#pragma once

// The placeholder character shapes, declared in one place so the menu and both
// levels draw the same Roary. The models are assembled from the Renderer's
// primitives and take a position, height, yaw and roll. Replacing the bodies of
// these functions updates every scene at once.

#include <glm/vec3.hpp>

class Renderer;

namespace mascots {
inline constexpr float kRoaryNaturalHeight = 1.42f;

void drawRoary(Renderer& renderer, const glm::vec3& feetPosition,
               float height = kRoaryNaturalHeight, float yawDegrees = 0.0f,
               float rollDegrees = 0.0f);

void drawEyePair(Renderer& renderer, const glm::vec3& center, float spacing,
                 float size, const glm::vec3& eyeColor,
                 const glm::vec3& pupilColor);

}
