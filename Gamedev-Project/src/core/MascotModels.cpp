// Builds Roary and Rocky the bull out of spheres, boxes, cylinders and pyramids
// at a fixed natural height, scaling the whole model from the height asked for.
// Roary also takes a roll, so he can stand on a wall or ceiling in the tunnel
// level. Every colour passes through the tint first, which is how one argument
// can wash a character pale without this file needing to know why.

#include "core/MascotModels.h"

#include <glm/common.hpp>

#include "core/Config.h"
#include "core/Renderer.h"

#include <glm/trigonometric.hpp>

#include <cmath>

namespace mascots {
void drawEyePair(Renderer& renderer, const glm::vec3& center, float spacing,
                 float size, const glm::vec3& eyeColor,
                 const glm::vec3& pupilColor) {
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -1.0f : 1.0f) * spacing * 0.5f;

        renderer.drawSphereAt(center + glm::vec3(x, 0.0f, 0.0f),
                              glm::vec3(size, size, size * 0.72f), eyeColor);
        renderer.drawSphereAt(center + glm::vec3(x, 0.0f, size * 0.30f),
                              glm::vec3(size * 0.46f, size * 0.52f, size * 0.30f),
                              pupilColor);
    }
}

void drawRoary(Renderer& renderer, const glm::vec3& feetPosition,
               float height, float yawDegrees, float rollDegrees,
               float tint, const glm::vec3& tintColor) {
    const float k = height / kRoaryNaturalHeight;

    const float yaw = glm::radians(yawDegrees);
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);

    const float roll = glm::radians(rollDegrees);
    const float cosRoll = std::cos(roll);
    const float sinRoll = std::sin(roll);

    const auto at = [&](float x, float y, float z) {
        const glm::vec3 local(x * k, y * k, z * k);
        const glm::vec3 turned(cosYaw * local.x + sinYaw * local.z,
                               local.y,
                               -sinYaw * local.x + cosYaw * local.z);
        return feetPosition + glm::vec3(turned.x * cosRoll - turned.y * sinRoll,
                                        turned.x * sinRoll + turned.y * cosRoll,
                                        turned.z);
    };
    const auto size = [&](float x, float y, float z) {
        return glm::vec3(x * k, y * k, z * k);
    };

    const glm::vec3 facing(0.0f, yawDegrees, rollDegrees);

    const auto shade = [&](const glm::vec3& c) {
        return glm::mix(c, tintColor, glm::clamp(tint, 0.0f, 1.0f));
    };

    const glm::vec3 fur   = shade(cfg::kPantherFur);
    const glm::vec3 cream = shade(cfg::kPantherCream);
    const glm::vec3 kit   = shade(cfg::kFiuKit);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.13f : 0.13f);
        renderer.drawCylinder(makeTransform(at(x, 0.14f, 0.0f),
                                            size(0.15f, 0.28f, 0.15f), facing), fur);
    }
    renderer.drawSphere(makeTransform(at(0.0f, 0.38f, 0.0f),
                                      size(0.50f, 0.28f, 0.44f), facing), kit);

    renderer.drawSphere(makeTransform(at(0.0f, 0.66f, 0.0f),
                                      size(0.54f, 0.50f, 0.46f), facing), kit);
    renderer.drawSphere(makeTransform(at(0.0f, 0.70f, 0.21f),
                                      size(0.24f, 0.11f, 0.05f), facing),
                        shade(cfg::kFiuKitTrim));

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.31f : 0.31f);
        renderer.drawSphere(makeTransform(at(x, 0.64f, 0.0f),
                                          size(0.15f, 0.34f, 0.15f), facing), fur);
    }

    renderer.drawSphere(makeTransform(at(0.0f, 1.06f, 0.0f),
                                      size(0.56f, 0.52f, 0.50f), facing), fur);
    renderer.drawSphere(makeTransform(at(0.0f, 0.97f, 0.19f),
                                      size(0.32f, 0.22f, 0.22f), facing), cream);
    renderer.drawSphere(makeTransform(at(0.0f, 1.03f, 0.30f),
                                      size(0.09f, 0.07f, 0.06f), facing),
                        shade(cfg::kPantherNose));

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.21f : 0.21f);
        renderer.drawSphere(makeTransform(at(x, 1.30f, 0.0f),
                                          size(0.19f, 0.21f, 0.14f), facing), fur);
        renderer.drawSphere(makeTransform(at(x, 1.30f, 0.05f),
                                          size(0.10f, 0.12f, 0.08f), facing),
                            shade(cfg::kPantherInner));
    }

    drawEyePair(renderer, at(0.0f, 1.12f, 0.20f), 0.23f * k, 0.11f * k,
                shade(cfg::kEyeWhite), shade(cfg::kEyeDark));
}

void drawBull(Renderer& renderer, const glm::vec3& feetPosition,
              float height, float yawDegrees, float tint,
              const glm::vec3& tintColor) {
    const float k = height / kBullNaturalHeight;

    const float yaw = glm::radians(yawDegrees);
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);

    const auto at = [&](float x, float y, float z) {
        const glm::vec3 local(x * k, y * k, z * k);
        return feetPosition + glm::vec3(cosYaw * local.x + sinYaw * local.z,
                                        local.y,
                                        -sinYaw * local.x + cosYaw * local.z);
    };
    const auto size = [&](float x, float y, float z) {
        return glm::vec3(x * k, y * k, z * k);
    };

    const glm::vec3 facing(0.0f, yawDegrees, 0.0f);

    const auto shade = [&](const glm::vec3& c) {
        return glm::mix(c, tintColor, glm::clamp(tint, 0.0f, 1.0f));
    };

    const glm::vec3 body   = shade(cfg::kBullBody);
    const glm::vec3 muzzle = shade(cfg::kBullMuzzle);
    const glm::vec3 horn   = shade(cfg::kBullHorn);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.13f : 0.13f);
        renderer.drawSphere(makeTransform(at(x, 0.05f, 0.03f),
                                          size(0.17f, 0.10f, 0.24f), facing),
                            shade(cfg::kBullHoof));
    }
    renderer.drawSphere(makeTransform(at(0.0f, 0.22f, 0.0f),
                                      size(0.44f, 0.30f, 0.38f), facing),
                        shade(cfg::kBullPants));

    renderer.drawSphere(makeTransform(at(0.0f, 0.54f, 0.0f),
                                      size(0.60f, 0.48f, 0.44f), facing),
                        shade(cfg::kBullJersey));
    renderer.drawSphere(makeTransform(at(0.0f, 0.58f, 0.21f),
                                      size(0.11f, 0.12f, 0.04f), facing),
                        shade(cfg::kFiuKitTrim));

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.33f : 0.33f);
        renderer.drawSphere(makeTransform(at(x, 0.52f, 0.0f),
                                          size(0.16f, 0.34f, 0.16f), facing),
                            body);
    }

    const float headY = 0.92f;
    renderer.drawSphere(makeTransform(at(0.0f, headY, 0.0f),
                                      size(0.44f, 0.40f, 0.42f), facing),
                        body);
    renderer.drawSphere(makeTransform(at(0.0f, headY - 0.07f, 0.26f),
                                      size(0.30f, 0.22f, 0.22f), facing),
                        muzzle);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.07f : 0.07f);
        renderer.drawSphere(makeTransform(at(x, headY - 0.09f, 0.29f),
                                          size(0.06f, 0.05f, 0.04f), facing),
                            shade(cfg::kEyeDark));
    }

    for (int side = 0; side < 2; ++side) {
        const float sign = (side == 0 ? -1.0f : 1.0f);

        renderer.drawSphere(makeTransform(at(sign * 0.20f, headY + 0.14f, 0.0f),
                                          size(0.16f, 0.15f, 0.15f), facing), horn);
        renderer.drawSphere(makeTransform(at(sign * 0.31f, headY + 0.26f, 0.0f),
                                          size(0.12f, 0.13f, 0.12f), facing), horn);
        renderer.drawSphere(makeTransform(at(sign * 0.34f, headY + 0.38f, 0.0f),
                                          size(0.08f, 0.10f, 0.08f), facing), horn);
    }

    renderer.drawSphere(makeTransform(at(0.0f, headY + 0.09f, 0.21f),
                                      size(0.34f, 0.07f, 0.08f), facing),
                        muzzle);
}

}
