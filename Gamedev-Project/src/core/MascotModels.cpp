#include "core/MascotModels.h"

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
               float height, float yawDegrees, float rollDegrees) {
    // Everything below is written at Roary's natural size; this rescales it.
    const float k = height / kRoaryNaturalHeight;

    const float yaw = glm::radians(yawDegrees);
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);

    const float roll = glm::radians(rollDegrees);
    const float cosRoll = std::cos(roll);
    const float sinRoll = std::sin(roll);

    // Turns a local offset (x right, y up, z forward) into a world position:
    // scale, then yaw about Y, then roll about Z, then translate.
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
    // Parts are turned as well as moved, or a muzzle would still point at +Z
    // after Roary turned to face sideways.
    const glm::vec3 facing(0.0f, yawDegrees, rollDegrees);

    const glm::vec3 fur   = cfg::kPantherFur;
    const glm::vec3 cream = cfg::kPantherCream;
    const glm::vec3 kit   = cfg::kFiuKit;

    // Legs and navy shorts.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.13f : 0.13f);
        renderer.drawCylinder(makeTransform(at(x, 0.14f, 0.0f),
                                            size(0.15f, 0.28f, 0.15f), facing), fur);
    }
    renderer.drawSphere(makeTransform(at(0.0f, 0.38f, 0.0f),
                                      size(0.50f, 0.28f, 0.44f), facing), kit);

    // Navy jersey over a round chest, with white lettering.
    renderer.drawSphere(makeTransform(at(0.0f, 0.66f, 0.0f),
                                      size(0.54f, 0.50f, 0.46f), facing), kit);
    renderer.drawSphere(makeTransform(at(0.0f, 0.70f, 0.21f),
                                      size(0.24f, 0.11f, 0.05f), facing),
                        cfg::kFiuKitTrim);

    // Arms.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.31f : 0.31f);
        renderer.drawSphere(makeTransform(at(x, 0.64f, 0.0f),
                                          size(0.15f, 0.34f, 0.15f), facing), fur);
    }

    // Head, muzzle and nose.
    renderer.drawSphere(makeTransform(at(0.0f, 1.06f, 0.0f),
                                      size(0.56f, 0.52f, 0.50f), facing), fur);
    renderer.drawSphere(makeTransform(at(0.0f, 0.97f, 0.19f),
                                      size(0.32f, 0.22f, 0.22f), facing), cream);
    renderer.drawSphere(makeTransform(at(0.0f, 1.03f, 0.30f),
                                      size(0.09f, 0.07f, 0.06f), facing),
                        cfg::kPantherNose);

    // Ears, with a darker inner surface.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.21f : 0.21f);
        renderer.drawSphere(makeTransform(at(x, 1.30f, 0.0f),
                                          size(0.19f, 0.21f, 0.14f), facing), fur);
        renderer.drawSphere(makeTransform(at(x, 1.30f, 0.05f),
                                          size(0.10f, 0.12f, 0.08f), facing),
                            cfg::kPantherInner);
    }

    drawEyePair(renderer, at(0.0f, 1.12f, 0.20f), 0.23f * k, 0.11f * k,
                cfg::kEyeWhite, cfg::kEyeDark);
}

} // namespace mascots
