// Draws the round tower, the lift and each floor's diorama, dimming locked floors
// and marking them with a padlock, and runs the menu on top of it. Moving the
// selection animates Roary's lift between floors, and mouse hovering and clicking
// are resolved by projecting each floor to the screen and testing the cursor
// against it.

#include "scenes/LevelSelectScene.h"

#include "core/Camera.h"
#include "core/GameProgress.h"
#include "core/Input.h"
#include "core/MascotModels.h"
#include "core/Renderer.h"
#include "core/SceneContext.h"
#include "core/SceneManager.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float kRadius = cfg::kTowerRadius;

constexpr float kLockZ = kRadius * 0.55f;

glm::vec3 pointOnCircle(float degrees, float radius, float y) {
    const float radians = glm::radians(degrees);
    return glm::vec3(std::sin(radians) * radius, y, std::cos(radians) * radius);
}

void drawWallArc(Renderer& renderer, float centerY, float height, float radius,
                 float thickness, const glm::vec3& color, int segments,
                 float openingDegrees) {
    const float covered = 360.0f - openingDegrees;
    const float step = covered / static_cast<float>(segments);

    const float width = 2.0f * radius * std::sin(glm::radians(step * 0.5f)) * 1.12f;

    for (int i = 0; i < segments; ++i) {
        const float angle = openingDegrees * 0.5f
                          + step * (static_cast<float>(i) + 0.5f);

        renderer.drawBox(
            makeTransform(pointOnCircle(angle, radius - thickness * 0.5f, centerY),
                          glm::vec3(width, height, thickness),
                          glm::vec3(0.0f, angle, 0.0f)),
            color);
    }
}

void drawMerlons(Renderer& renderer, float y, float radius,
                 const glm::vec3& color, int count, float openingDegrees) {
    const float covered = 360.0f - openingDegrees;
    const float step = covered / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        const float angle = openingDegrees * 0.5f
                          + step * (static_cast<float>(i) + 0.5f);

        renderer.drawBox(
            makeTransform(pointOnCircle(angle, radius - cfg::kWallThickness * 0.5f, y),
                          glm::vec3(cfg::kMerlonWidth, cfg::kMerlonHeight,
                                    cfg::kWallThickness * 1.05f),
                          glm::vec3(0.0f, angle, 0.0f)),
            color);
    }
}

void drawRing(Renderer& renderer, float y, float radius, float thickness,
              const glm::vec3& color, float alpha) {
    constexpr int kRingSegments = 30;
    constexpr float kStep = 360.0f / static_cast<float>(kRingSegments);
    const float width = 2.0f * radius * std::sin(glm::radians(kStep * 0.5f)) * 1.15f;

    for (int i = 0; i < kRingSegments; ++i) {
        const float angle = kStep * static_cast<float>(i);
        renderer.drawBox(
            makeTransform(pointOnCircle(angle, radius, y),
                          glm::vec3(width, thickness, thickness),
                          glm::vec3(0.0f, angle, 0.0f)),
            color, alpha, 1.0f);
    }
}

glm::mat4 hangingTransform(const glm::vec3& pivot, const glm::vec3& size,
                           float rollDegrees) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, pivot);
    model = glm::rotate(model, glm::radians(rollDegrees), glm::vec3(0, 0, 1));

    model = glm::translate(model, glm::vec3(0.0f, -size.y * 0.5f, 0.0f));
    model = glm::scale(model, size);
    return model;
}

}

bool LevelSelectScene::init() {
    selectedFloor_ = 0;

    for (int floor = cfg::kFloorCount - 1; floor >= 0; --floor) {
        if (ctx().progress.isUnlocked(floor)) {
            selectedFloor_ = floor;
            break;
        }
    }

    roaryY_ = cfg::floorCenterY(selectedFloor_);
    elapsedTime_ = 0.0f;
    lockedFlashTimer_ = 0.0f;
    hoveredFloor_ = -1;

    return true;
}

float LevelSelectScene::px(float designPixels) const {
    return designPixels * uiScale_;
}

float LevelSelectScene::floorPickHeight(int floorIndex) const {
    const bool isTopFloor = (floorIndex == cfg::kFloorCount - 1);
    return isTopFloor ? cfg::kTopFloorPickHeight : cfg::kFloorHeight;
}

glm::vec3 LevelSelectScene::shade(const glm::vec3& color, bool locked) const {
    if (!locked) return color;
    return glm::mix(color, cfg::kLockedGrey, cfg::kLockedFade);
}

void LevelSelectScene::moveSelection(int delta) {
    const int next = selectedFloor_ + delta;

    if (next < 0 || next >= cfg::kFloorCount) {
        return;
    }
    selectedFloor_ = next;

    lockedFlashTimer_ = 0.0f;
}

void LevelSelectScene::tryEnterSelectedFloor() {
    if (!ctx().progress.isUnlocked(selectedFloor_)) {
        lockedFlashTimer_ = 0.75f;
        return;
    }

    ctx().manager.requestScene(sceneIdForFloor(selectedFloor_));
}

void LevelSelectScene::handleInput(const Input& input) {
    refreshFloorScreenRects();

    if (input.wasPressed(GLFW_KEY_W) || input.wasPressed(GLFW_KEY_UP)) {
        moveSelection(+1);
    }
    if (input.wasPressed(GLFW_KEY_S) || input.wasPressed(GLFW_KEY_DOWN)) {
        moveSelection(-1);
    }

    const glm::vec2 mousePosition = input.mousePosition();
    const glm::vec2 mouseDelta = mousePosition - lastMousePosition_;
    const bool mouseMoved = std::abs(mouseDelta.x) + std::abs(mouseDelta.y) > 1.0f;
    lastMousePosition_ = mousePosition;

    hoveredFloor_ = floorUnderMouse(mousePosition);
    if (mouseMoved && hoveredFloor_ >= 0 && hoveredFloor_ != selectedFloor_) {
        selectedFloor_ = hoveredFloor_;
        lockedFlashTimer_ = 0.0f;
    }

    if (input.wasMouseClicked() && hoveredFloor_ >= 0) {
        selectedFloor_ = hoveredFloor_;
        tryEnterSelectedFloor();
        return;
    }

    if (input.wasPressed(GLFW_KEY_ENTER) || input.wasPressed(GLFW_KEY_SPACE)) {
        tryEnterSelectedFloor();
        return;
    }

    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        ctx().manager.requestQuit();
        return;
    }

    if (input.wasPressed(GLFW_KEY_U)) {
        ctx().progress.unlockAll();
    }
}

void LevelSelectScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    if (lockedFlashTimer_ > 0.0f) {
        lockedFlashTimer_ = std::max(0.0f, lockedFlashTimer_ - deltaTime);
    }

    const float targetY = cfg::floorCenterY(selectedFloor_);
    const float blend = 1.0f - std::exp(-cfg::kRoaryMoveSpeed * deltaTime);
    roaryY_ += (targetY - roaryY_) * blend;
}

void LevelSelectScene::refreshFloorScreenRects() {
    const glm::mat4& viewProjection = ctx().renderer.viewProjection();
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        const float bottomY = cfg::floorCenterY(floor) - cfg::kFloorHeight * 0.5f;
        const float topY = bottomY + floorPickHeight(floor);

        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        bool anyVisible = false;

        for (int corner = 0; corner < 8; ++corner) {
            const glm::vec3 worldCorner{
                (corner & 1) ? kRadius : -kRadius,
                (corner & 2) ? topY : bottomY,
                (corner & 4) ? kRadius : -kRadius,
            };

            glm::vec3 screenPoint(0.0f);
            if (!worldToScreen(worldCorner, viewProjection, width, height, screenPoint)) {
                continue;
            }
            anyVisible = true;
            minX = std::min(minX, screenPoint.x);
            minY = std::min(minY, screenPoint.y);
            maxX = std::max(maxX, screenPoint.x);
            maxY = std::max(maxY, screenPoint.y);
        }

        floorScreenRects_[static_cast<std::size_t>(floor)] = anyVisible
            ? glm::vec4(minX, minY, maxX, maxY)
            : glm::vec4(0.0f);
    }
}

int LevelSelectScene::floorUnderMouse(const glm::vec2& mousePosition) const {
    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        const glm::vec4& rect = floorScreenRects_[static_cast<std::size_t>(floor)];
        if (rect.z <= rect.x || rect.w <= rect.y) {
            continue;
        }
        if (mousePosition.x >= rect.x && mousePosition.x <= rect.z &&
            mousePosition.y >= rect.y && mousePosition.y <= rect.w) {
            return floor;
        }
    }
    return -1;
}

void LevelSelectScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    renderer.drawBackground(cfg::kBgTop, cfg::kBgBottom, cfg::kBgGlow,
                            cfg::kBgGlowPos, cfg::kBgGlowRadius,
                            cfg::kBgGlowStrength);

    const float sway = std::sin(elapsedTime_ * cfg::kCameraSwaySpeed)
                     * cfg::kCameraSwayAmount;
    glm::vec3 cameraPosition = cfg::kMenuCameraPos;
    cameraPosition.x += sway;
    cameraPosition.y += std::cos(elapsedTime_ * cfg::kCameraSwaySpeed * 0.7f)
                      * cfg::kCameraSwayAmount * 0.4f;

    Camera camera(cameraPosition, cfg::kMenuCameraTarget);
    camera.setFovDegrees(cfg::kMenuCameraFovDeg);
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawTower();
    drawRoary();

    drawUi();
}

void LevelSelectScene::drawTower() {
    Renderer& renderer = ctx().renderer;

    renderer.drawCylinderAt(glm::vec3(0.0f, cfg::kTowerBaseY - 0.55f, 0.0f),
                            glm::vec3((kRadius + 0.75f) * 2.0f, 1.1f,
                                      (kRadius + 0.75f) * 2.0f),
                            cfg::kTowerBase);

    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        drawFloorShell(floor);
        drawFloorProps(floor);

        if (!ctx().progress.isUnlocked(floor)) {
            drawLockIcon(floor);
        }
        if (floor == selectedFloor_) {
            drawSelectionFrame(floor);
        }
    }

}

void LevelSelectScene::drawFloorShell(int floorIndex) {
    Renderer& renderer = ctx().renderer;

    const bool locked = !ctx().progress.isUnlocked(floorIndex);
    const bool isTopFloor = (floorIndex == cfg::kFloorCount - 1);
    const float centerY = cfg::floorCenterY(floorIndex);
    const float halfHeight = cfg::kFloorHeight * 0.5f;
    const float floorY = centerY - halfHeight;

    glm::vec3 wallColor = cfg::kTowerStone;
    switch (floorIndex) {
        case 0: wallColor = cfg::kF1Wall; break;
        case 1: wallColor = cfg::kF2Wall; break;
        case 2: wallColor = cfg::kF3Wall; break;
        case 3: wallColor = cfg::kF4Wall; break;
        default: break;
    }

    const float discDiameter = (kRadius + cfg::kSlabOverhang) * 2.0f;
    const glm::vec3 stone = shade(cfg::kTowerStone, locked);

    renderer.drawCylinderAt(glm::vec3(0.0f, floorY, 0.0f),
                            glm::vec3(discDiameter, cfg::kSlabThickness, discDiameter),
                            stone);

    if (isTopFloor) {
        const float parapetBase = floorY + cfg::kSlabThickness * 0.5f;

        drawWallArc(renderer, parapetBase + cfg::kParapetHeight * 0.5f,
                    cfg::kParapetHeight, kRadius, cfg::kWallThickness,
                    shade(cfg::kTowerStoneDark, locked),
                    cfg::kWallSegments, cfg::kWallOpeningDegrees);

        drawMerlons(renderer,
                    parapetBase + cfg::kParapetHeight + cfg::kMerlonHeight * 0.5f,
                    kRadius, shade(cfg::kTowerMerlon, locked),
                    cfg::kMerlonCount, cfg::kWallOpeningDegrees);
        return;
    }

    drawWallArc(renderer, centerY, cfg::kFloorHeight, kRadius, cfg::kWallThickness,
                shade(wallColor, locked), cfg::kWallSegments, cfg::kWallOpeningDegrees);

    renderer.drawCylinderAt(glm::vec3(0.0f, centerY + halfHeight, 0.0f),
                            glm::vec3(discDiameter, cfg::kSlabThickness, discDiameter),
                            stone);

    for (int side = 0; side < 2; ++side) {
        const float angle = (side == 0 ? -1.0f : 1.0f) * cfg::kWallOpeningDegrees * 0.5f;
        renderer.drawBox(
            makeTransform(pointOnCircle(angle, kRadius - cfg::kWallThickness * 0.5f, centerY),
                          glm::vec3(cfg::kPillarWidth, cfg::kFloorHeight,
                                    cfg::kWallThickness * 1.15f),
                          glm::vec3(0.0f, angle, 0.0f)),
            shade(cfg::kTowerStoneDark, locked));
    }
}

void LevelSelectScene::drawFloorProps(int floorIndex) {
    const bool locked = !ctx().progress.isUnlocked(floorIndex);
    const float centerY = cfg::floorCenterY(floorIndex);

    switch (floorIndex) {
        case 0: drawPlatformerProps(centerY, locked); break;
        case 1: drawTunnelProps(centerY, locked);     break;
        case 2: drawStealthProps(centerY, locked);    break;
        case 3: drawBossProps(centerY, locked);       break;
        default: break;
    }
}

void LevelSelectScene::drawPlatformerProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    struct Platform { float x; float y; glm::vec3 color; };
    const Platform platforms[] = {
        {-2.10f, -0.58f, cfg::kF1BlockWarm},
        {-0.30f, -0.05f, cfg::kF1BlockCool},
        { 1.55f,  0.42f, cfg::kF1BlockWarm},
    };

    for (const Platform& platform : platforms) {
        renderer.drawBoxAt(
            glm::vec3(platform.x, centerY + platform.y, cfg::kPropZ),
            glm::vec3(1.25f, 0.28f, 0.80f),
            shade(platform.color, locked));
    }

    const float bob = std::sin(elapsedTime_ * 2.2f) * 0.07f;
    renderer.drawBox(
        makeTransform(glm::vec3(2.55f, centerY + 0.80f + bob, cfg::kPropZ),
                      glm::vec3(0.34f, 0.34f, 0.12f),
                      glm::vec3(0.0f, elapsedTime_ * 90.0f, 0.0f)),
        shade(cfg::kF1Coin, locked), 1.0f, locked ? 0.0f : 0.45f);
}

void LevelSelectScene::drawTunnelProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    renderer.drawBoxAt(glm::vec3(0.0f, centerY, 0.02f),
                       glm::vec3(cfg::kFloorWidth - 1.3f, cfg::kFloorHeight - 0.55f, 0.08f),
                       shade(cfg::kF2Tunnel, locked));

    constexpr int kRingCount = 3;
    const float ringZ[kRingCount]     = { 0.95f, 0.60f, 0.25f };
    const float ringScale[kRingCount] = { 1.00f, 0.72f, 0.46f };

    for (int ring = 0; ring < kRingCount; ++ring) {
        const float halfW = (cfg::kFloorWidth - 1.9f) * 0.5f * ringScale[ring];
        const float halfH = (cfg::kFloorHeight - 0.75f) * 0.5f * ringScale[ring];
        const float bar = 0.16f * ringScale[ring];

        const float fade = 1.0f - static_cast<float>(ring) * 0.28f;
        const glm::vec3 ringColor = (ring % 2 == 0) ? cfg::kF2RimGlow : cfg::kF2Accent;
        const glm::vec3 finalColor = shade(ringColor * fade, locked);
        const float emissive = locked ? 0.0f : 0.75f;

        const float z = ringZ[ring];

        renderer.drawBoxAt(glm::vec3(0.0f, centerY + halfH, z),
                           glm::vec3(halfW * 2.0f + bar, bar, bar),
                           finalColor, 1.0f, emissive);
        renderer.drawBoxAt(glm::vec3(0.0f, centerY - halfH, z),
                           glm::vec3(halfW * 2.0f + bar, bar, bar),
                           finalColor, 1.0f, emissive);

        renderer.drawBoxAt(glm::vec3(-halfW, centerY, z),
                           glm::vec3(bar, halfH * 2.0f, bar),
                           finalColor, 1.0f, emissive);
        renderer.drawBoxAt(glm::vec3( halfW, centerY, z),
                           glm::vec3(bar, halfH * 2.0f, bar),
                           finalColor, 1.0f, emissive);
    }
}

void LevelSelectScene::drawStealthProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    const float floorY = centerY - cfg::kFloorHeight * 0.5f;
    const float ceilingY = centerY + cfg::kFloorHeight * 0.5f;

    const float guardX[2] = {-1.95f, 1.35f};
    for (const float x : guardX) {
        const glm::vec3 guardColor = shade(cfg::kF3Guard, locked);

        renderer.drawBoxAt(glm::vec3(x, floorY + 0.58f, cfg::kPropZ),
                           glm::vec3(0.42f, 0.86f, 0.34f), guardColor);
        renderer.drawBoxAt(glm::vec3(x, floorY + 1.16f, cfg::kPropZ),
                           glm::vec3(0.30f, 0.30f, 0.30f), guardColor);
    }

    renderer.setDepthWrite(false);

    const float beamLength = cfg::kFloorHeight * 0.92f;
    const float sweep = std::sin(elapsedTime_ * cfg::kF3SweepSpeed) * cfg::kF3SweepDeg;

    const float mountX[2] = {-1.10f, 2.05f};
    for (int beam = 0; beam < 2; ++beam) {
        const float angle = (beam == 0) ? sweep : -sweep;
        const glm::vec3 pivot(mountX[beam], ceilingY - 0.14f, cfg::kPropZ);

        renderer.drawBoxAt(pivot, glm::vec3(0.26f, 0.18f, 0.26f),
                           shade(cfg::kTowerStoneDark, locked));

        renderer.drawPyramid(
            hangingTransform(pivot, glm::vec3(1.05f, beamLength, 0.75f), angle),
            shade(cfg::kF3Light, locked),
            locked ? cfg::kF3LightAlpha * 0.40f : cfg::kF3LightAlpha,
            1.0f);
    }

    renderer.setDepthWrite(true);
}

void LevelSelectScene::drawBossProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    const float deckY = centerY - cfg::kFloorHeight * 0.5f
                      + cfg::kSlabThickness * 0.5f;

    renderer.drawCylinderAt(
        glm::vec3(0.0f, deckY + cfg::kBossDaisHeight * 0.5f, 0.0f),
        glm::vec3(cfg::kBossDaisRadius * 2.0f, cfg::kBossDaisHeight,
                  cfg::kBossDaisRadius * 2.0f),
        shade(cfg::kTowerStoneDark, locked));

    const float floorY = deckY + cfg::kBossDaisHeight;

    constexpr float kBossZ = 1.70f;

    drawPawlinaInCage(glm::vec3(-1.90f, floorY, kBossZ - 0.1f), locked);

    drawOwl (glm::vec3(-0.35f, floorY, kBossZ), locked);
    drawIbis(glm::vec3( 0.95f, floorY, kBossZ), locked);

    mascots::drawBull(renderer, glm::vec3( 2.10f, floorY, kBossZ),
                      mascots::kBullNaturalHeight, 0.0f,
                      locked ? cfg::kLockedFade : 0.0f, cfg::kLockedGrey);
}

void LevelSelectScene::drawPawlinaInCage(const glm::vec3& feetPosition, bool locked) {
    Renderer& renderer = ctx().renderer;

    const glm::vec3 fur   = shade(cfg::kPantherFur, locked);
    const glm::vec3 cream = shade(cfg::kPantherCream, locked);
    const glm::vec3 kit   = shade(cfg::kFiuKit, locked);
    const glm::vec3 gold  = shade(cfg::kPawlinaTrim, locked);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.09f : 0.09f);
        renderer.drawCylinderAt(feetPosition + glm::vec3(x, 0.12f, 0.0f),
                                glm::vec3(0.11f, 0.24f, 0.11f), fur);
    }

    renderer.drawCylinderAt(feetPosition + glm::vec3(0.0f, 0.34f, 0.0f),
                            glm::vec3(0.46f, 0.22f, 0.40f), kit);
    renderer.drawCylinderAt(feetPosition + glm::vec3(0.0f, 0.24f, 0.0f),
                            glm::vec3(0.47f, 0.05f, 0.41f), gold);
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.56f, 0.0f),
                          glm::vec3(0.36f, 0.40f, 0.32f), kit);

    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.60f, 0.15f),
                          glm::vec3(0.19f, 0.09f, 0.05f), shade(cfg::kFiuKitTrim, locked));

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.23f : 0.23f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.56f, 0.0f),
                              glm::vec3(0.13f, 0.30f, 0.13f), fur);
    }

    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.90f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.40f, 0.38f, 0.36f), fur);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.07f, 0.15f),
                          glm::vec3(0.24f, 0.17f, 0.18f), cream);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.02f, 0.25f),
                          glm::vec3(0.07f, 0.05f, 0.05f),
                          shade(cfg::kPantherNose, locked));

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.16f : 0.16f);
        renderer.drawSphereAt(head + glm::vec3(x, 0.19f, 0.0f),
                              glm::vec3(0.15f, 0.17f, 0.11f), fur);
        renderer.drawSphereAt(head + glm::vec3(x, 0.19f, 0.04f),
                              glm::vec3(0.08f, 0.10f, 0.06f),
                              shade(cfg::kPantherInner, locked));
    }

    renderer.drawSphereAt(head + glm::vec3(-0.22f, 0.26f, 0.0f),
                          glm::vec3(0.15f, 0.10f, 0.08f), kit);
    renderer.drawSphereAt(head + glm::vec3(-0.22f, 0.26f, 0.03f),
                          glm::vec3(0.06f, 0.06f, 0.05f), gold);

    mascots::drawEyePair(renderer, head + glm::vec3(0.0f, 0.05f, 0.16f), 0.19f, 0.09f,
                shade(cfg::kEyeWhite, locked), shade(cfg::kEyeDark, locked));

    const glm::vec3 barColor = shade(cfg::kF4CageBar, locked);
    const float cageHeight = 1.42f;
    const float cageHalfWidth = 0.44f;
    const float cageZ = 0.34f;

    constexpr int kBarCount = 5;
    for (int bar = 0; bar < kBarCount; ++bar) {
        const float t = static_cast<float>(bar) / static_cast<float>(kBarCount - 1);
        const float x = -cageHalfWidth + t * (cageHalfWidth * 2.0f);
        renderer.drawCylinderAt(feetPosition + glm::vec3(x, cageHeight * 0.5f, cageZ),
                                glm::vec3(0.055f, cageHeight, 0.055f), barColor);
    }

    renderer.drawBoxAt(feetPosition + glm::vec3(0.0f, cageHeight, cageZ),
                       glm::vec3(cageHalfWidth * 2.0f + 0.06f, 0.07f, 0.07f), barColor);
    renderer.drawBoxAt(feetPosition + glm::vec3(0.0f, 0.03f, cageZ),
                       glm::vec3(cageHalfWidth * 2.0f + 0.06f, 0.07f, 0.07f), barColor);
}

void LevelSelectScene::drawOwl(const glm::vec3& feetPosition, bool locked) {
    Renderer& renderer = ctx().renderer;

    const glm::vec3 body   = shade(cfg::kOwlBody, locked);
    const glm::vec3 cream  = shade(cfg::kOwlCream, locked);
    const glm::vec3 jersey = shade(cfg::kOwlJersey, locked);

    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.16f, 0.0f),
                          glm::vec3(0.40f, 0.28f, 0.34f), shade(cfg::kOwlPants, locked));
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.46f, 0.0f),
                          glm::vec3(0.48f, 0.46f, 0.40f), jersey);

    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.50f, 0.19f),
                          glm::vec3(0.13f, 0.11f, 0.04f), shade(cfg::kFiuKitTrim, locked));

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.27f : 0.27f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.46f, 0.0f),
                              glm::vec3(0.14f, 0.32f, 0.20f), body);
    }

    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.84f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.52f, 0.44f, 0.44f), body);

    renderer.drawSphereAt(head + glm::vec3(-0.17f, 0.10f, 0.14f),
                          glm::vec3(0.07f, 0.05f, 0.04f), cream);
    renderer.drawSphereAt(head + glm::vec3(0.19f, 0.06f, 0.13f),
                          glm::vec3(0.06f, 0.04f, 0.04f), cream);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.20f : 0.20f);
        renderer.drawSphereAt(head + glm::vec3(x, 0.20f, 0.0f),
                              glm::vec3(0.16f, 0.19f, 0.13f), body);
    }

    mascots::drawEyePair(renderer, head + glm::vec3(0.0f, 0.04f, 0.17f), 0.24f, 0.14f,
                shade(cfg::kOwlEye, locked), shade(cfg::kEyeDark, locked));

    for (int side = 0; side < 2; ++side) {
        const float sign = (side == 0 ? -1.0f : 1.0f);
        renderer.drawSphere(
            makeTransform(head + glm::vec3(sign * 0.14f, 0.16f, 0.17f),
                          glm::vec3(0.26f, 0.07f, 0.08f),
                          glm::vec3(0.0f, 0.0f, sign * -16.0f)),
            shade(cfg::kOwlBrow, locked), 1.0f, locked ? 0.0f : 0.30f);
    }

    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.07f, 0.21f),
                          glm::vec3(0.11f, 0.15f, 0.13f), cream);
}

void LevelSelectScene::drawIbis(const glm::vec3& feetPosition, bool locked) {
    Renderer& renderer = ctx().renderer;

    const glm::vec3 body   = shade(cfg::kIbisBody, locked);
    const glm::vec3 bill   = shade(cfg::kIbisBill, locked);
    const glm::vec3 jersey = shade(cfg::kIbisJersey, locked);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.10f : 0.10f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.04f, 0.05f),
                              glm::vec3(0.17f, 0.07f, 0.26f),
                              shade(cfg::kIbisFeet, locked));
        renderer.drawCylinderAt(feetPosition + glm::vec3(x, 0.19f, 0.0f),
                                glm::vec3(0.07f, 0.28f, 0.07f), bill);
    }

    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.40f, 0.0f),
                          glm::vec3(0.38f, 0.32f, 0.34f), body);
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.60f, 0.0f),
                          glm::vec3(0.40f, 0.34f, 0.36f), jersey);
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.62f, 0.17f),
                          glm::vec3(0.10f, 0.11f, 0.04f), shade(cfg::kFiuKitTrim, locked));

    for (int side = 0; side < 2; ++side) {
        const float sign = (side == 0 ? -1.0f : 1.0f);
        renderer.drawSphere(
            makeTransform(feetPosition + glm::vec3(sign * 0.27f, 0.66f, 0.0f),
                          glm::vec3(0.12f, 0.30f, 0.18f),
                          glm::vec3(0.0f, 0.0f, sign * 28.0f)),
            body);
    }

    renderer.drawCylinderAt(feetPosition + glm::vec3(0.0f, 0.84f, 0.0f),
                            glm::vec3(0.15f, 0.20f, 0.15f), body);
    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.98f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.27f, 0.25f, 0.26f), body);

    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.03f, 0.21f),
                          glm::vec3(0.13f, 0.08f, 0.34f), bill);

    mascots::drawEyePair(renderer, head + glm::vec3(0.0f, 0.06f, 0.10f), 0.15f, 0.07f,
                shade(cfg::kEyeWhite, locked), shade(cfg::kEyeDark, locked));

    renderer.drawCylinderAt(head + glm::vec3(0.0f, 0.17f, -0.01f),
                            glm::vec3(0.30f, 0.06f, 0.30f), body);
    renderer.drawCylinderAt(head + glm::vec3(0.0f, 0.21f, -0.01f),
                            glm::vec3(0.22f, 0.05f, 0.22f),
                            shade(cfg::kIbisGreen, locked));
}

void LevelSelectScene::drawSelectionFrame(int floorIndex) {
    Renderer& renderer = ctx().renderer;

    const float floorY = cfg::floorCenterY(floorIndex) - cfg::kFloorHeight * 0.5f;

    const float pulse = 0.55f + 0.45f
        * (0.5f + 0.5f * std::sin(elapsedTime_ * cfg::kSelectPulseSpeed));

    const float upperRingOffset = floorPickHeight(floorIndex);

    const float ringRadius = kRadius + cfg::kSlabOverhang + 0.16f;

    drawRing(renderer, floorY, ringRadius, cfg::kSelectFrameThick,
             cfg::kSelectFrame, pulse);
    drawRing(renderer, floorY + upperRingOffset, ringRadius,
             cfg::kSelectFrameThick, cfg::kSelectFrame, pulse);
}

void LevelSelectScene::drawLockIcon(int floorIndex) {
    Renderer& renderer = ctx().renderer;

    const float centerY = cfg::floorCenterY(floorIndex);
    const glm::vec3 center(0.0f, centerY, kLockZ);
    const glm::vec3 color = cfg::kF4CageBar;

    renderer.drawBoxAt(center, glm::vec3(0.46f, 0.38f, 0.16f), color);

    renderer.drawBoxAt(center + glm::vec3(0.0f, 0.34f, 0.0f),
                       glm::vec3(0.30f, 0.07f, 0.10f), color);
    renderer.drawBoxAt(center + glm::vec3(-0.12f, 0.26f, 0.0f),
                       glm::vec3(0.07f, 0.20f, 0.10f), color);
    renderer.drawBoxAt(center + glm::vec3( 0.12f, 0.26f, 0.0f),
                       glm::vec3(0.07f, 0.20f, 0.10f), color);

    renderer.drawBoxAt(center + glm::vec3(0.0f, -0.02f, 0.09f),
                       glm::vec3(0.09f, 0.14f, 0.04f), cfg::kLockedGrey);
}

void LevelSelectScene::drawRoary() {
    Renderer& renderer = ctx().renderer;

    const float bob = std::sin(elapsedTime_ * cfg::kRoaryBobSpeed)
                    * cfg::kRoaryBobAmount;

    const float feetY = roaryY_ - cfg::kFloorHeight * 0.5f
                      + cfg::kSlabThickness + bob;
    const glm::vec3 feet(cfg::kRoaryOffsetX, feetY, cfg::kRoaryOffsetZ);

    renderer.drawCylinderAt(feet + glm::vec3(0.0f, -0.08f, 0.0f),
                            glm::vec3(1.30f, 0.16f, 1.20f), cfg::kRoaryAccent);

    mascots::drawRoary(renderer, feet, cfg::kRoarySize.y, 0.0f);
}

void LevelSelectScene::drawUi() {
    Renderer& renderer = ctx().renderer;
    TextRenderer& text = renderer.text();

    const float width = static_cast<float>(ctx().framebufferWidth);
    const float height = static_cast<float>(ctx().framebufferHeight);
    const float centerX = width * 0.5f;

    text.begin(ctx().framebufferWidth, ctx().framebufferHeight);

    text.drawCentered("ROARY'S ADVENTURE", centerX, px(30.0f),
                      px(cfg::kTitleScale), cfg::kTextPrimary);
    text.drawCentered("SAVE PAWLINA!", centerX, px(92.0f),
                      px(cfg::kSubtitleScale), cfg::kFiuGold);

    const std::string progressLabel =
        "FLOORS CLEARED " + std::to_string(ctx().progress.completedCount()) +
        "/" + std::to_string(cfg::kFloorCount);
    text.draw(progressLabel,
              width - px(24.0f) - TextRenderer::measureWidth(progressLabel, px(cfg::kHintScale)),
              px(24.0f), px(cfg::kHintScale), cfg::kTextDim);

    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        const bool unlocked = ctx().progress.isUnlocked(floor);
        const bool completed = ctx().progress.isCompleted(floor);

        glm::vec3 screenPoint(0.0f);

        const glm::vec3 anchor(kRadius + 1.05f, cfg::floorCenterY(floor), 0.9f);
        if (!worldToScreen(anchor, renderer.viewProjection(),
                           ctx().framebufferWidth, ctx().framebufferHeight,
                           screenPoint)) {
            continue;
        }

        const float labelScale = px(cfg::kFloorLabelScale);
        const bool isSelected = (floor == selectedFloor_);

        glm::vec3 labelColor = cfg::kTextDim;
        if (!unlocked)        labelColor = cfg::kTextLocked;
        else if (completed)   labelColor = cfg::kTextComplete;
        else if (isSelected)  labelColor = cfg::kTextPrimary;

        const std::string label = std::string(isSelected ? "> " : "  ")
                                + cfg::kFloorInfo[floor].shortLabel;

        const float lineHeight = TextRenderer::measureHeight(labelScale);
        const float gap = labelScale * 1.5f;

        text.draw(label, screenPoint.x,
                  screenPoint.y - lineHeight - gap,
                  labelScale, labelColor);

        const char* stateText = !unlocked ? "[LOCKED]"
                              : completed ? "[CLEARED]"
                                          : "[OPEN]";
        text.draw(stateText, screenPoint.x, screenPoint.y + gap,
                  labelScale * 0.85f, labelColor);
    }

    const float panelHeight = px(190.0f);
    const float panelY = height - panelHeight - px(18.0f);
    const float panelX = px(50.0f);
    const float panelWidth = width - panelX * 2.0f;

    text.fillRect(panelX, panelY, panelWidth, panelHeight,
                  cfg::kBgBottom, 0.93f);

    text.fillRect(panelX, panelY, panelWidth, px(3.0f), cfg::kFiuGold, 0.85f);

    const cfg::FloorInfo& info = cfg::kFloorInfo[selectedFloor_];
    const bool unlocked = ctx().progress.isUnlocked(selectedFloor_);
    const bool completed = ctx().progress.isCompleted(selectedFloor_);

    text.drawCentered(info.name, centerX, panelY + px(22.0f),
                      px(cfg::kFloorNameScale),
                      unlocked ? cfg::kTextPrimary : cfg::kTextDim);

    text.drawCentered(info.description, centerX, panelY + px(66.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    std::string statusText;
    glm::vec3 statusColor = cfg::kTextPrimary;
    float statusAlpha = 1.0f;

    if (!unlocked) {
        statusText = "LOCKED - CLEAR THE FLOOR BELOW FIRST";
        statusColor = cfg::kTextLocked;

        if (lockedFlashTimer_ > 0.0f) {
            statusAlpha = 0.45f + 0.55f * std::abs(std::sin(lockedFlashTimer_ * 18.0f));
        } else {
            statusAlpha = 0.85f;
        }
    } else if (completed) {
        statusText = "CLEARED - PRESS ENTER TO PLAY IT AGAIN";
        statusColor = cfg::kTextComplete;
    } else {
        statusText = "PRESS ENTER TO CLIMB";
        statusColor = cfg::kFiuGold;
    }

    text.drawCentered(statusText, centerX, panelY + px(100.0f),
                      px(cfg::kHintScale), statusColor, statusAlpha);

    text.drawCentered("W/S OR UP/DOWN - CHOOSE FLOOR     ENTER OR SPACE - START     ESC - QUIT",
                      centerX, panelY + px(136.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.drawCentered("MOUSE - HOVER AND CLICK A FLOOR     U - DEBUG UNLOCK ALL",
                      centerX, panelY + px(162.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
