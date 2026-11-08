#pragma once
//=============================================================================
// MascotModels - the placeholder character shapes, in one place so every scene
// draws the same Roary.
//
// Each model is assembled from the Renderer's rounded primitives. They are
// stand-ins: when real models exist, replace the bodies of these functions and
// every scene picks up the change at once.
//=============================================================================

#include <glm/vec3.hpp>

class Renderer;

namespace mascots {

// Roary's natural height in world units when `height` is left at this value.
// All his proportions are written against it and scaled from there.
inline constexpr float kRoaryNaturalHeight = 1.42f;

// Rocky's natural height, the same way. He is a shade taller than Roary and a
// good deal wider, which is what makes him read as a threat.
inline constexpr float kBullNaturalHeight = 1.52f;

//-----------------------------------------------------------------------------
// Draws Roary standing with his feet at `feetPosition`.
//   - height:      total height in world units
//   - yawDegrees:  which way he faces, 0 = toward +Z (toward the camera in
//                  the level-select scene)
//   - rollDegrees: rotation about Z, so he can stand on a wall or ceiling in
//                  the tunnel level (0 = upright)
//   - tint:        blends his colours toward `tintColor`, 0 = normal
//                  (the boss floor washes him pale blue while invincible;
//                  it has a default, so other callers can leave it out)
//-----------------------------------------------------------------------------
void drawRoary(Renderer& renderer, const glm::vec3& feetPosition,
               float height = kRoaryNaturalHeight, float yawDegrees = 0.0f,
               float rollDegrees = 0.0f, float tint = 0.0f,
               const glm::vec3& tintColor = glm::vec3(1.0f));

//-----------------------------------------------------------------------------
// Both eyes at once: a pale eyeball with a dark pupil sitting slightly proud
// of it. `center` is the midpoint between the two eyes. Shared because every
// character needs it.
//-----------------------------------------------------------------------------
void drawEyePair(Renderer& renderer, const glm::vec3& center, float spacing,
                 float size, const glm::vec3& eyeColor,
                 const glm::vec3& pupilColor);

//-----------------------------------------------------------------------------
// USF's Rocky.
//   - yawDegrees: which way he faces, 0 = down +Z (same as everything else)
//   - tint:       works the same as Roary's
//-----------------------------------------------------------------------------
void drawBull(Renderer& renderer, const glm::vec3& feetPosition,
              float height = kBullNaturalHeight, float yawDegrees = 0.0f,
              float tint = 0.0f, const glm::vec3& tintColor = glm::vec3(1.0f));

} // namespace mascots
