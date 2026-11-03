#include "scenes/Characters.h"

#include "core/Config.h"
#include "core/Renderer.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>

namespace {

// Roary's proportions, shared by every scene that draws him. These used to be
// floor-3 constants; they moved here because they describe the CHARACTER, not
// the stealth level, and the boss floor wants exactly the same panther.
constexpr float kBobAmount   = 0.055f;  // how much he bounces as he walks
constexpr float kShadowRadius = 0.42f;  // the blob shadow under his feet
constexpr glm::vec3 kShadowColor {0.020f, 0.047f, 0.055f};
constexpr float kShadowAlpha = 0.55f;

} // namespace

float shortestAngleDelta(float fromDeg, float toDeg) {
    // Add 540 (= 360 + 180) before the modulo so the result is always
    // positive going in, then subtract the 180 back off. That shifts the
    // answer into -180..180 without needing an if-statement for each way
    // the angle could have wrapped.
    return std::fmod(toDeg - fromDeg + 540.0f, 360.0f) - 180.0f;
}

void drawRoary(Renderer& renderer,
               const glm::vec3& feetPosition,
               float facingDeg,
               float walkCycle,
               bool walking,
               float scale,
               float tint,
               const glm::vec3& tintColor) {
    // The walk bounce, and a slight lean into the direction of travel. Both
    // stop when he does.
    const float bob = walking
        ? std::abs(std::sin(walkCycle)) * kBobAmount
        : 0.0f;

    const glm::vec3 feet = feetPosition + glm::vec3(0.0f, bob, 0.0f);

    // --- blob shadow ---------------------------------------------------------
    // Drawn first and flat on the deck. Translucent, so depth writes are off
    // or it would punch a hole in the floor behind it.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(feetPosition.x, 0.015f, feetPosition.z),
                            glm::vec3(kShadowRadius * 2.3f, 0.02f,
                                      kShadowRadius * 2.3f),
                            kShadowColor, kShadowAlpha, 1.0f);
    renderer.setDepthWrite(true);

    // --- the panther ---------------------------------------------------------
    // Built the same way as the Roary on the level-select screen: squashed
    // spheres at a natural height of 1.42 units, scaled by k. The difference
    // here is that he turns, so every part is positioned by rotating its local
    // offset about Y and is drawn with that same rotation.
    const float k = scale;
    const float facingRad = glm::radians(facingDeg);
    const float sinF = std::sin(facingRad);
    const float cosF = std::cos(facingRad);

    // Local space: +Z is the direction he faces, +X is his left.
    const auto at = [&](float x, float y, float z) {
        return feet + glm::vec3((x * cosF + z * sinF) * k,
                                 y * k,
                                (-x * sinF + z * cosF) * k);
    };
    const auto size = [&](float x, float y, float z) {
        return glm::vec3(x * k, y * k, z * k);
    };
    const glm::vec3 spin(0.0f, facingDeg, 0.0f);

    const auto sphere = [&](const glm::vec3& center, const glm::vec3& sz,
                            const glm::vec3& color) {
        renderer.drawSphere(makeTransform(center, sz, spin), color);
    };

    // Every colour passes through the tint, so a scene can flash him white
    // (or red) on a hit without this function needing to know why.
    const auto shade = [&](const glm::vec3& c) {
        return glm::mix(c, tintColor, glm::clamp(tint, 0.0f, 1.0f));
    };

    const glm::vec3 fur   = shade(cfg::kPantherFur);
    const glm::vec3 cream = shade(cfg::kPantherCream);
    const glm::vec3 kit   = shade(cfg::kFiuKit);

    // Legs swing in opposite phase so the walk reads from above.
    const float stride = walking ? std::sin(walkCycle) * 0.16f : 0.0f;
    for (int side = 0; side < 2; ++side) {
        const float x    = (side == 0 ? -0.13f : 0.13f);
        const float step = (side == 0 ? stride : -stride);
        renderer.drawCylinder(
            makeTransform(at(x, 0.14f, step), size(0.15f, 0.28f, 0.15f), spin), fur);
    }

    // Navy shorts, then the jersey over a round chest with white lettering.
    sphere(at(0.0f, 0.38f, 0.0f), size(0.50f, 0.28f, 0.44f), kit);
    sphere(at(0.0f, 0.66f, 0.0f), size(0.54f, 0.50f, 0.46f), kit);
    sphere(at(0.0f, 0.70f, 0.21f), size(0.24f, 0.11f, 0.05f), shade(cfg::kFiuKitTrim));

    // Arms, swinging opposite their leg on the same side.
    for (int side = 0; side < 2; ++side) {
        const float x    = (side == 0 ? -0.31f : 0.31f);
        const float step = (side == 0 ? -stride : stride);
        sphere(at(x, 0.64f, step * 0.8f), size(0.15f, 0.34f, 0.15f), fur);
    }

    // Head, muzzle, nose.
    sphere(at(0.0f, 1.06f, 0.0f),  size(0.56f, 0.52f, 0.50f), fur);
    sphere(at(0.0f, 0.97f, 0.19f), size(0.32f, 0.22f, 0.22f), cream);
    sphere(at(0.0f, 1.03f, 0.30f), size(0.09f, 0.07f, 0.06f), shade(cfg::kPantherNose));

    // Ears. Worth the four draws: from this overhead camera they are the part
    // of him that most clearly shows which way he is pointing.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.21f : 0.21f);
        sphere(at(x, 1.30f, 0.00f), size(0.19f, 0.21f, 0.14f), fur);
        sphere(at(x, 1.30f, 0.05f), size(0.10f, 0.12f, 0.08f), shade(cfg::kPantherInner));
    }

    // Eyes.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.115f : 0.115f);
        sphere(at(x, 1.12f, 0.20f), size(0.13f, 0.14f, 0.08f), shade(cfg::kEyeWhite));
        sphere(at(x, 1.12f, 0.24f), size(0.07f, 0.08f, 0.05f), shade(cfg::kEyeDark));
    }
}
