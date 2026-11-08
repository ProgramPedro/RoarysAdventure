// Builds Roary out of spheres, boxes and pyramids at a fixed natural height and
// scales the whole model from the height asked for. The yaw turns him to face any
// direction and the roll lets him stand on a wall or ceiling in the tunnel level.
// Also draws the shared eye pair that every character uses.

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

    const glm::vec3 fur   = cfg::kPantherFur;
    const glm::vec3 cream = cfg::kPantherCream;
    const glm::vec3 kit   = cfg::kFiuKit;

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
                        cfg::kFiuKitTrim);

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
                        cfg::kPantherNose);

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

}
