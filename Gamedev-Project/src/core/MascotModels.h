#pragma once

// The placeholder character shapes, declared in one place so every scene draws
// the same Roary and the same Rocky. Each model is assembled from the Renderer's
// primitives and takes a position, height and facing, plus a tint that washes its
// colours toward another colour - the boss floor uses that to show invincibility.
// Replacing the bodies of these functions updates every scene at once.

#include <glm/vec3.hpp>

class Renderer;

namespace mascots {
inline constexpr float kRoaryNaturalHeight = 1.42f;

inline constexpr float kBullNaturalHeight = 1.52f;

void drawRoary(Renderer& renderer, const glm::vec3& feetPosition,
               float height = kRoaryNaturalHeight, float yawDegrees = 0.0f,
               float rollDegrees = 0.0f, float tint = 0.0f,
               const glm::vec3& tintColor = glm::vec3(1.0f));

void drawEyePair(Renderer& renderer, const glm::vec3& center, float spacing,
                 float size, const glm::vec3& eyeColor,
                 const glm::vec3& pupilColor);

void drawBull(Renderer& renderer, const glm::vec3& feetPosition,
              float height = kBullNaturalHeight, float yawDegrees = 0.0f,
              float tint = 0.0f, const glm::vec3& tintColor = glm::vec3(1.0f));

}
