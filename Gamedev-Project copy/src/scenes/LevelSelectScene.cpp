#include "scenes/LevelSelectScene.h"

#include "core/Camera.h"
#include "core/GameProgress.h"
#include "core/Input.h"
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

// The tower is round, so one radius replaces the old width/depth pair.
constexpr float kRadius = cfg::kTowerRadius;

// Where the lock icon floats inside the open front of a floor.
constexpr float kLockZ = kRadius * 0.55f;

// Converts an angle measured from +Z (toward the camera) around toward +X into
// a point on the tower's circle. Rotating a box by that same angle about Y
// makes its front face point outward, which is how the wall segments are laid
// out below.
glm::vec3 pointOnCircle(float degrees, float radius, float y) {
    const float radians = glm::radians(degrees);
    return glm::vec3(std::sin(radians) * radius, y, std::cos(radians) * radius);
}

// Draws a stretch of curved wall as a ring of flat segments, leaving
// `openingDegrees` open at the front so you can see into the room.
void drawWallArc(Renderer& renderer, float centerY, float height, float radius,
                 float thickness, const glm::vec3& color, int segments,
                 float openingDegrees) {
    const float covered = 360.0f - openingDegrees;
    const float step = covered / static_cast<float>(segments);

    // Each segment is a flat chord across its slice of the circle. The 1.12
    // factor widens them slightly so neighbours overlap and no seams show.
    const float width = 2.0f * radius * std::sin(glm::radians(step * 0.5f)) * 1.12f;

    for (int i = 0; i < segments; ++i) {
        // Start at the edge of the opening and sweep around the back.
        const float angle = openingDegrees * 0.5f
                          + step * (static_cast<float>(i) + 0.5f);

        renderer.drawBox(
            makeTransform(pointOnCircle(angle, radius - thickness * 0.5f, centerY),
                          glm::vec3(width, height, thickness),
                          glm::vec3(0.0f, angle, 0.0f)),
            color);
    }
}

// The blocks along the top of the parapet that make it read as a castle.
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

// A thin ring of blocks all the way around the tower, used to highlight the
// selected floor. A rectangle would look wrong on a round tower.
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

// Builds a beam / limb that hangs from a pivot point and rotates about it,
// rather than about its own center. Used for searchlights and horns.
glm::mat4 hangingTransform(const glm::vec3& pivot, const glm::vec3& size,
                           float rollDegrees) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, pivot);
    model = glm::rotate(model, glm::radians(rollDegrees), glm::vec3(0, 0, 1));
    // Shift down by half the length so the TOP of the shape sits at the pivot.
    model = glm::translate(model, glm::vec3(0.0f, -size.y * 0.5f, 0.0f));
    model = glm::scale(model, size);
    return model;
}

// Both eyes at once: a pale eyeball with a dark pupil sitting slightly proud
// of it. `center` is the midpoint between the two eyes.
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

} // namespace

//=============================================================================
// Lifecycle
//=============================================================================

bool LevelSelectScene::init() {
    selectedFloor_ = 0;

    // Open on the highest floor the player has actually unlocked, so returning
    // to the menu after beating a floor puts the cursor where they left off.
    for (int floor = cfg::kFloorCount - 1; floor >= 0; --floor) {
        if (ctx().progress.isUnlocked(floor)) {
            selectedFloor_ = floor;
            break;
        }
    }

    // Start Roary already at the selected floor instead of sliding in.
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
    // Enclosed floors are as tall as the room. The open castle top only counts
    // up to the battlements plus the figures standing on it.
    const bool isTopFloor = (floorIndex == cfg::kFloorCount - 1);
    return isTopFloor ? cfg::kTopFloorPickHeight : cfg::kFloorHeight;
}

glm::vec3 LevelSelectScene::shade(const glm::vec3& color, bool locked) const {
    if (!locked) return color;
    return glm::mix(color, cfg::kLockedGrey, cfg::kLockedFade);
}

//=============================================================================
// Input
//=============================================================================

void LevelSelectScene::moveSelection(int delta) {
    const int next = selectedFloor_ + delta;

    // Clamp instead of wrapping: the tower has a top and a bottom, and running
    // off either end should just stop.
    if (next < 0 || next >= cfg::kFloorCount) {
        return;
    }
    selectedFloor_ = next;

    // Any successful move clears a pending LOCKED flash.
    lockedFlashTimer_ = 0.0f;
}

void LevelSelectScene::tryEnterSelectedFloor() {
    if (!ctx().progress.isUnlocked(selectedFloor_)) {
        // Flash the LOCKED label rather than silently ignoring the press, so
        // the player knows the input registered.
        lockedFlashTimer_ = 0.75f;
        return;
    }

    // Only a REQUEST: this scene keeps running for the rest of this frame and
    // SceneManager swaps it out once the frame has been presented.
    ctx().manager.requestScene(sceneIdForFloor(selectedFloor_));
}

void LevelSelectScene::handleInput(const Input& input) {
    // The screen rectangles are needed for mouse hit-testing, and they depend
    // on the camera, so refresh them before reading the mouse.
    refreshFloorScreenRects();

    // --- keyboard: move up and down the tower --------------------------------
    // Up means "a higher floor", which is a HIGHER index.
    if (input.wasPressed(GLFW_KEY_W) || input.wasPressed(GLFW_KEY_UP)) {
        moveSelection(+1);
    }
    if (input.wasPressed(GLFW_KEY_S) || input.wasPressed(GLFW_KEY_DOWN)) {
        moveSelection(-1);
    }

    // --- mouse: hover selects, click enters ----------------------------------
    // Hover only takes over once the mouse has actually MOVED. Without this,
    // a cursor that happens to be resting over a floor would keep yanking the
    // selection back every frame while the player is using the keyboard.
    const glm::vec2 mousePosition = input.mousePosition();
    const glm::vec2 mouseDelta = mousePosition - lastMousePosition_;
    const bool mouseMoved = std::abs(mouseDelta.x) + std::abs(mouseDelta.y) > 1.0f;
    lastMousePosition_ = mousePosition;

    hoveredFloor_ = floorUnderMouse(mousePosition);
    if (mouseMoved && hoveredFloor_ >= 0 && hoveredFloor_ != selectedFloor_) {
        selectedFloor_ = hoveredFloor_;
        lockedFlashTimer_ = 0.0f;
    }
    // A click always works, moved or not.
    if (input.wasMouseClicked() && hoveredFloor_ >= 0) {
        selectedFloor_ = hoveredFloor_;
        tryEnterSelectedFloor();
        return;
    }

    // --- confirm / quit ------------------------------------------------------
    if (input.wasPressed(GLFW_KEY_ENTER) || input.wasPressed(GLFW_KEY_SPACE)) {
        tryEnterSelectedFloor();
        return;
    }

    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        ctx().manager.requestQuit();
        return;
    }

    // --- debug ---------------------------------------------------------------
    if (input.wasPressed(GLFW_KEY_U)) {
        ctx().progress.unlockAll();
    }
}

//=============================================================================
// Update
//=============================================================================

void LevelSelectScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    if (lockedFlashTimer_ > 0.0f) {
        lockedFlashTimer_ = std::max(0.0f, lockedFlashTimer_ - deltaTime);
    }

    // Ease Roary toward the selected floor. The 1 - exp(-k*dt) form gives a
    // smooth slide that behaves the same at any frame rate, unlike a plain
    // lerp by a fixed fraction.
    const float targetY = cfg::floorCenterY(selectedFloor_);
    const float blend = 1.0f - std::exp(-cfg::kRoaryMoveSpeed * deltaTime);
    roaryY_ += (targetY - roaryY_) * blend;
}

//=============================================================================
// Screen-space helpers (mouse picking and floor labels)
//=============================================================================

void LevelSelectScene::refreshFloorScreenRects() {
    const glm::mat4& viewProjection = ctx().renderer.viewProjection();
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        // Use the floor's VISIBLE height. The top floor is open, so its full
        // floor height would let you select it by pointing at empty sky.
        const float bottomY = cfg::floorCenterY(floor) - cfg::kFloorHeight * 0.5f;
        const float topY = bottomY + floorPickHeight(floor);

        // Project the eight corners of the box that encloses this floor's drum
        // and take the extremes. That gives a screen rectangle which still
        // fits correctly even though the camera views the tower at an angle.
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
            : glm::vec4(0.0f);  // off screen: an empty rect never gets hit
    }
}

int LevelSelectScene::floorUnderMouse(const glm::vec2& mousePosition) const {
    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        const glm::vec4& rect = floorScreenRects_[static_cast<std::size_t>(floor)];
        if (rect.z <= rect.x || rect.w <= rect.y) {
            continue;  // empty / off-screen
        }
        if (mousePosition.x >= rect.x && mousePosition.x <= rect.z &&
            mousePosition.y >= rect.y && mousePosition.y <= rect.w) {
            return floor;
        }
    }
    return -1;
}

//=============================================================================
// Render
//=============================================================================

void LevelSelectScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    // --- 1. the FIU blue-and-gold backdrop -----------------------------------
    renderer.drawBackground(cfg::kBgTop, cfg::kBgBottom, cfg::kBgGlow,
                            cfg::kBgGlowPos, cfg::kBgGlowRadius,
                            cfg::kBgGlowStrength);

    // --- 2. camera ------------------------------------------------------------
    // A very slow drift keeps the menu from looking like a still image.
    const float sway = std::sin(elapsedTime_ * cfg::kCameraSwaySpeed)
                     * cfg::kCameraSwayAmount;
    glm::vec3 cameraPosition = cfg::kMenuCameraPos;
    cameraPosition.x += sway;
    cameraPosition.y += std::cos(elapsedTime_ * cfg::kCameraSwaySpeed * 0.7f)
                      * cfg::kCameraSwayAmount * 0.4f;

    Camera camera(cameraPosition, cfg::kMenuCameraTarget);
    camera.setFovDegrees(cfg::kMenuCameraFovDeg);
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    // --- 3. the tower and Roary ----------------------------------------------
    drawTower();
    drawRoary();

    // --- 4. the text and panels on top ---------------------------------------
    drawUi();
}

void LevelSelectScene::drawTower() {
    Renderer& renderer = ctx().renderer;

    // Round foundation the tower stands on.
    renderer.drawCylinderAt(glm::vec3(0.0f, cfg::kTowerBaseY - 0.55f, 0.0f),
                            glm::vec3((kRadius + 0.75f) * 2.0f, 1.1f,
                                      (kRadius + 0.75f) * 2.0f),
                            cfg::kTowerBase);

    // Each floor: the stone shell first, then the themed contents inside it,
    // then the lock icon or the selection ring.
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

    // There is no roof: the tower ends in the open battlements drawn as part
    // of the top floor's shell.
}

void LevelSelectScene::drawFloorShell(int floorIndex) {
    Renderer& renderer = ctx().renderer;

    const bool locked = !ctx().progress.isUnlocked(floorIndex);
    const bool isTopFloor = (floorIndex == cfg::kFloorCount - 1);
    const float centerY = cfg::floorCenterY(floorIndex);
    const float halfHeight = cfg::kFloorHeight * 0.5f;
    const float floorY = centerY - halfHeight;

    // The curved back wall carries each floor's theme color, so the room
    // already reads as "platformer" or "tunnel" behind the props.
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

    // Every floor stands on a round disc.
    renderer.drawCylinderAt(glm::vec3(0.0f, floorY, 0.0f),
                            glm::vec3(discDiameter, cfg::kSlabThickness, discDiameter),
                            stone);

    if (isTopFloor) {
        // --- the open castle top -------------------------------------------
        // No ceiling and no full-height wall: just a low parapet ringed with
        // battlements, so the boss fight happens under open sky.
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

    // --- an enclosed round room ---------------------------------------------
    drawWallArc(renderer, centerY, cfg::kFloorHeight, kRadius, cfg::kWallThickness,
                shade(wallColor, locked), cfg::kWallSegments, cfg::kWallOpeningDegrees);

    // Ceiling disc.
    renderer.drawCylinderAt(glm::vec3(0.0f, centerY + halfHeight, 0.0f),
                            glm::vec3(discDiameter, cfg::kSlabThickness, discDiameter),
                            stone);

    // Two pillars at the edges of the opening, framing the view inside.
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

//-----------------------------------------------------------------------------
// FLOOR 1 - Mario-style platformer: bright blocks and a spinning coin.
//-----------------------------------------------------------------------------
void LevelSelectScene::drawPlatformerProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    // Platforms at staggered heights, reading as a little jumping route.
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

    // A coin spinning over the last platform. Turning it 45 degrees on Y makes
    // the cube read as a diamond, and the bob gives the floor some life.
    const float bob = std::sin(elapsedTime_ * 2.2f) * 0.07f;
    renderer.drawBox(
        makeTransform(glm::vec3(2.55f, centerY + 0.80f + bob, cfg::kPropZ),
                      glm::vec3(0.34f, 0.34f, 0.12f),
                      glm::vec3(0.0f, elapsedTime_ * 90.0f, 0.0f)),
        shade(cfg::kF1Coin, locked), 1.0f, locked ? 0.0f : 0.45f);
}

//-----------------------------------------------------------------------------
// FLOOR 2 - "Run"-style tunnel: nested rings receding into the dark.
//-----------------------------------------------------------------------------
void LevelSelectScene::drawTunnelProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    // A darker panel just in front of the wall: the mouth of the tunnel.
    renderer.drawBoxAt(glm::vec3(0.0f, centerY, 0.02f),
                       glm::vec3(cfg::kFloorWidth - 1.3f, cfg::kFloorHeight - 0.55f, 0.08f),
                       shade(cfg::kF2Tunnel, locked));

    // Three rings, each smaller, dimmer and further back than the last. The
    // shrinking creates the illusion of a tunnel running away from the camera.
    // All three sit IN FRONT of the back wall, or the inner two would be
    // swallowed by it.
    constexpr int kRingCount = 3;
    const float ringZ[kRingCount]     = { 0.95f, 0.60f, 0.25f };
    const float ringScale[kRingCount] = { 1.00f, 0.72f, 0.46f };

    for (int ring = 0; ring < kRingCount; ++ring) {
        const float halfW = (cfg::kFloorWidth - 1.9f) * 0.5f * ringScale[ring];
        const float halfH = (cfg::kFloorHeight - 0.75f) * 0.5f * ringScale[ring];
        const float bar = 0.16f * ringScale[ring];

        // Rings fade out with distance, and alternate between the violet rim
        // color and the cyan accent so the tunnel looks lit from inside.
        const float fade = 1.0f - static_cast<float>(ring) * 0.28f;
        const glm::vec3 ringColor = (ring % 2 == 0) ? cfg::kF2RimGlow : cfg::kF2Accent;
        const glm::vec3 finalColor = shade(ringColor * fade, locked);
        const float emissive = locked ? 0.0f : 0.75f;

        const float z = ringZ[ring];

        // Top and bottom bars.
        renderer.drawBoxAt(glm::vec3(0.0f, centerY + halfH, z),
                           glm::vec3(halfW * 2.0f + bar, bar, bar),
                           finalColor, 1.0f, emissive);
        renderer.drawBoxAt(glm::vec3(0.0f, centerY - halfH, z),
                           glm::vec3(halfW * 2.0f + bar, bar, bar),
                           finalColor, 1.0f, emissive);

        // Left and right bars.
        renderer.drawBoxAt(glm::vec3(-halfW, centerY, z),
                           glm::vec3(bar, halfH * 2.0f, bar),
                           finalColor, 1.0f, emissive);
        renderer.drawBoxAt(glm::vec3( halfW, centerY, z),
                           glm::vec3(bar, halfH * 2.0f, bar),
                           finalColor, 1.0f, emissive);
    }
}

//-----------------------------------------------------------------------------
// FLOOR 3 - Stealth: sweeping searchlights and guard silhouettes.
//-----------------------------------------------------------------------------
void LevelSelectScene::drawStealthProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    const float floorY = centerY - cfg::kFloorHeight * 0.5f;
    const float ceilingY = centerY + cfg::kFloorHeight * 0.5f;

    // Two guards: a body box with a smaller head box on top.
    const float guardX[2] = {-1.95f, 1.35f};
    for (const float x : guardX) {
        const glm::vec3 guardColor = shade(cfg::kF3Guard, locked);

        renderer.drawBoxAt(glm::vec3(x, floorY + 0.58f, cfg::kPropZ),
                           glm::vec3(0.42f, 0.86f, 0.34f), guardColor);
        renderer.drawBoxAt(glm::vec3(x, floorY + 1.16f, cfg::kPropZ),
                           glm::vec3(0.30f, 0.30f, 0.30f), guardColor);
    }

    // Searchlight beams hanging from the ceiling. Translucent shapes must not
    // write depth, or they would hide the geometry behind them.
    renderer.setDepthWrite(false);

    const float beamLength = cfg::kFloorHeight * 0.92f;
    const float sweep = std::sin(elapsedTime_ * cfg::kF3SweepSpeed) * cfg::kF3SweepDeg;

    const float mountX[2] = {-1.10f, 2.05f};
    for (int beam = 0; beam < 2; ++beam) {
        // The second beam sweeps the opposite way, so they cross.
        const float angle = (beam == 0) ? sweep : -sweep;
        const glm::vec3 pivot(mountX[beam], ceilingY - 0.14f, cfg::kPropZ);

        // The housing the beam swings from.
        renderer.drawBoxAt(pivot, glm::vec3(0.26f, 0.18f, 0.26f),
                           shade(cfg::kTowerStoneDark, locked));

        // The cone of light. A pyramid hung from its apex gives exactly the
        // right shape: narrow at the lamp, spreading out toward the floor.
        renderer.drawPyramid(
            hangingTransform(pivot, glm::vec3(1.05f, beamLength, 0.75f), angle),
            shade(cfg::kF3Light, locked),
            locked ? cfg::kF3LightAlpha * 0.40f : cfg::kF3LightAlpha,
            1.0f);
    }

    renderer.setDepthWrite(true);
}

//-----------------------------------------------------------------------------
// TOP FLOOR - Boss fight: Pawlina caged, three rival mascots guarding her.
//-----------------------------------------------------------------------------
void LevelSelectScene::drawBossProps(float centerY, bool locked) {
    Renderer& renderer = ctx().renderer;

    const float deckY = centerY - cfg::kFloorHeight * 0.5f
                      + cfg::kSlabThickness * 0.5f;

    // The arena dais. It also lifts everyone above the deck rim, which from
    // this camera angle would otherwise cut the figures off at the knee.
    renderer.drawCylinderAt(
        glm::vec3(0.0f, deckY + cfg::kBossDaisHeight * 0.5f, 0.0f),
        glm::vec3(cfg::kBossDaisRadius * 2.0f, cfg::kBossDaisHeight,
                  cfg::kBossDaisRadius * 2.0f),
        shade(cfg::kTowerStoneDark, locked));

    const float floorY = deckY + cfg::kBossDaisHeight;

    // The camera looks slightly UP at the tower, so any flat surface hides
    // whatever stands in the middle of it. Standing the group near the front
    // of the dais keeps their legs in view - the further back they are, the
    // more of them the dais's own rim cuts off.
    constexpr float kBossZ = 1.70f;

    // Pawlina is held on the left; the three rivals stand between her and the
    // open front of the arena.
    drawPawlinaInCage(glm::vec3(-1.90f, floorY, kBossZ - 0.1f), locked);

    drawOwl (glm::vec3(-0.35f, floorY, kBossZ), locked);
    drawIbis(glm::vec3( 0.95f, floorY, kBossZ), locked);
    drawBull(glm::vec3( 2.10f, floorY, kBossZ), locked);
}

void LevelSelectScene::drawPawlinaInCage(const glm::vec3& feetPosition, bool locked) {
    Renderer& renderer = ctx().renderer;

    const glm::vec3 fur   = shade(cfg::kPantherFur, locked);
    const glm::vec3 cream = shade(cfg::kPantherCream, locked);
    const glm::vec3 kit   = shade(cfg::kFiuKit, locked);
    const glm::vec3 gold  = shade(cfg::kPawlinaTrim, locked);

    // Legs.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.09f : 0.09f);
        renderer.drawCylinderAt(feetPosition + glm::vec3(x, 0.12f, 0.0f),
                                glm::vec3(0.11f, 0.24f, 0.11f), fur);
    }

    // The navy cheer dress: a flared skirt with a gold hem, and a fitted top.
    renderer.drawCylinderAt(feetPosition + glm::vec3(0.0f, 0.34f, 0.0f),
                            glm::vec3(0.46f, 0.22f, 0.40f), kit);
    renderer.drawCylinderAt(feetPosition + glm::vec3(0.0f, 0.24f, 0.0f),
                            glm::vec3(0.47f, 0.05f, 0.41f), gold);
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.56f, 0.0f),
                          glm::vec3(0.36f, 0.40f, 0.32f), kit);

    // White lettering across the chest.
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.60f, 0.15f),
                          glm::vec3(0.19f, 0.09f, 0.05f), shade(cfg::kFiuKitTrim, locked));

    // Arms.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.23f : 0.23f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.56f, 0.0f),
                              glm::vec3(0.13f, 0.30f, 0.13f), fur);
    }

    // Head, muzzle and nose.
    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.90f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.40f, 0.38f, 0.36f), fur);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.07f, 0.15f),
                          glm::vec3(0.24f, 0.17f, 0.18f), cream);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.02f, 0.25f),
                          glm::vec3(0.07f, 0.05f, 0.05f),
                          shade(cfg::kPantherNose, locked));

    // Ears, with a darker inner surface.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.16f : 0.16f);
        renderer.drawSphereAt(head + glm::vec3(x, 0.19f, 0.0f),
                              glm::vec3(0.15f, 0.17f, 0.11f), fur);
        renderer.drawSphereAt(head + glm::vec3(x, 0.19f, 0.04f),
                              glm::vec3(0.08f, 0.10f, 0.06f),
                              shade(cfg::kPantherInner, locked));
    }

    // Her bow, worn over the left ear.
    renderer.drawSphereAt(head + glm::vec3(-0.22f, 0.26f, 0.0f),
                          glm::vec3(0.15f, 0.10f, 0.08f), kit);
    renderer.drawSphereAt(head + glm::vec3(-0.22f, 0.26f, 0.03f),
                          glm::vec3(0.06f, 0.06f, 0.05f), gold);

    drawEyePair(renderer, head + glm::vec3(0.0f, 0.05f, 0.16f), 0.19f, 0.09f,
                shade(cfg::kEyeWhite, locked), shade(cfg::kEyeDark, locked));

    // The cage: vertical bars across the front, with a rail top and bottom.
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

    // White football pants, then the blue jersey over a round owl body.
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.16f, 0.0f),
                          glm::vec3(0.40f, 0.28f, 0.34f), shade(cfg::kOwlPants, locked));
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.46f, 0.0f),
                          glm::vec3(0.48f, 0.46f, 0.40f), jersey);

    // Jersey number.
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.50f, 0.19f),
                          glm::vec3(0.13f, 0.11f, 0.04f), shade(cfg::kFiuKitTrim, locked));

    // Wings tucked at his sides.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.27f : 0.27f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.46f, 0.0f),
                              glm::vec3(0.14f, 0.32f, 0.20f), body);
    }

    // A wide, flat head - the main thing that says "owl" at this size.
    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.84f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.52f, 0.44f, 0.44f), body);

    // Cream speckles across the chest and brow.
    renderer.drawSphereAt(head + glm::vec3(-0.17f, 0.10f, 0.14f),
                          glm::vec3(0.07f, 0.05f, 0.04f), cream);
    renderer.drawSphereAt(head + glm::vec3(0.19f, 0.06f, 0.13f),
                          glm::vec3(0.06f, 0.04f, 0.04f), cream);

    // Ear tufts.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.20f : 0.20f);
        renderer.drawSphereAt(head + glm::vec3(x, 0.20f, 0.0f),
                              glm::vec3(0.16f, 0.19f, 0.13f), body);
    }

    // Big blue eyes, and the bright orange brows that are his signature. The
    // brows angle down toward the middle, which is what gives him that scowl.
    drawEyePair(renderer, head + glm::vec3(0.0f, 0.04f, 0.17f), 0.24f, 0.14f,
                shade(cfg::kOwlEye, locked), shade(cfg::kEyeDark, locked));

    for (int side = 0; side < 2; ++side) {
        const float sign = (side == 0 ? -1.0f : 1.0f);
        renderer.drawSphere(
            makeTransform(head + glm::vec3(sign * 0.14f, 0.16f, 0.17f),
                          glm::vec3(0.26f, 0.07f, 0.08f),
                          glm::vec3(0.0f, 0.0f, sign * -16.0f)),
            shade(cfg::kOwlBrow, locked), 1.0f, locked ? 0.0f : 0.30f);
    }

    // Beak.
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.07f, 0.21f),
                          glm::vec3(0.11f, 0.15f, 0.13f), cream);
}

void LevelSelectScene::drawIbis(const glm::vec3& feetPosition, bool locked) {
    Renderer& renderer = ctx().renderer;

    const glm::vec3 body   = shade(cfg::kIbisBody, locked);
    const glm::vec3 bill   = shade(cfg::kIbisBill, locked);
    const glm::vec3 jersey = shade(cfg::kIbisJersey, locked);

    // Big yellow webbed feet on orange stilt legs.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.10f : 0.10f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.04f, 0.05f),
                              glm::vec3(0.17f, 0.07f, 0.26f),
                              shade(cfg::kIbisFeet, locked));
        renderer.drawCylinderAt(feetPosition + glm::vec3(x, 0.19f, 0.0f),
                                glm::vec3(0.07f, 0.28f, 0.07f), bill);
    }

    // White feathered body under an orange jersey.
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.40f, 0.0f),
                          glm::vec3(0.38f, 0.32f, 0.34f), body);
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.60f, 0.0f),
                          glm::vec3(0.40f, 0.34f, 0.36f), jersey);
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.62f, 0.17f),
                          glm::vec3(0.10f, 0.11f, 0.04f), shade(cfg::kFiuKitTrim, locked));

    // Wings held out, the way he is always posing.
    for (int side = 0; side < 2; ++side) {
        const float sign = (side == 0 ? -1.0f : 1.0f);
        renderer.drawSphere(
            makeTransform(feetPosition + glm::vec3(sign * 0.27f, 0.66f, 0.0f),
                          glm::vec3(0.12f, 0.30f, 0.18f),
                          glm::vec3(0.0f, 0.0f, sign * 28.0f)),
            body);
    }

    // Long neck and small head.
    renderer.drawCylinderAt(feetPosition + glm::vec3(0.0f, 0.84f, 0.0f),
                            glm::vec3(0.15f, 0.20f, 0.15f), body);
    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.98f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.27f, 0.25f, 0.26f), body);

    // The long flat orange bill.
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.03f, 0.21f),
                          glm::vec3(0.13f, 0.08f, 0.34f), bill);

    drawEyePair(renderer, head + glm::vec3(0.0f, 0.06f, 0.10f), 0.15f, 0.07f,
                shade(cfg::kEyeWhite, locked), shade(cfg::kEyeDark, locked));

    // His little sailor hat, with the green band.
    renderer.drawCylinderAt(head + glm::vec3(0.0f, 0.17f, -0.01f),
                            glm::vec3(0.30f, 0.06f, 0.30f), body);
    renderer.drawCylinderAt(head + glm::vec3(0.0f, 0.21f, -0.01f),
                            glm::vec3(0.22f, 0.05f, 0.22f),
                            shade(cfg::kIbisGreen, locked));
}

void LevelSelectScene::drawBull(const glm::vec3& feetPosition, bool locked) {
    Renderer& renderer = ctx().renderer;

    const glm::vec3 body   = shade(cfg::kBullBody, locked);
    const glm::vec3 muzzle = shade(cfg::kBullMuzzle, locked);
    const glm::vec3 horn   = shade(cfg::kBullHorn, locked);

    // Black boots, vegas gold pants.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.13f : 0.13f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.05f, 0.03f),
                              glm::vec3(0.17f, 0.10f, 0.24f), shade(cfg::kBullHoof, locked));
    }
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.22f, 0.0f),
                          glm::vec3(0.44f, 0.30f, 0.38f), shade(cfg::kBullPants, locked));

    // Heavy green-jerseyed chest - he is the widest of the three.
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.54f, 0.0f),
                          glm::vec3(0.60f, 0.48f, 0.44f), shade(cfg::kBullJersey, locked));
    renderer.drawSphereAt(feetPosition + glm::vec3(0.0f, 0.58f, 0.21f),
                          glm::vec3(0.11f, 0.12f, 0.04f), shade(cfg::kFiuKitTrim, locked));

    // Grey arms.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.33f : 0.33f);
        renderer.drawSphereAt(feetPosition + glm::vec3(x, 0.52f, 0.0f),
                              glm::vec3(0.16f, 0.34f, 0.16f), body);
    }

    // Head and snout.
    const glm::vec3 head = feetPosition + glm::vec3(0.0f, 0.92f, 0.0f);
    renderer.drawSphereAt(head, glm::vec3(0.44f, 0.40f, 0.42f), body);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.09f, 0.19f),
                          glm::vec3(0.30f, 0.22f, 0.22f), muzzle);

    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.07f : 0.07f);
        renderer.drawSphereAt(head + glm::vec3(x, -0.09f, 0.29f),
                              glm::vec3(0.06f, 0.05f, 0.04f), shade(cfg::kEyeDark, locked));
    }

    // The gold horns. Three shrinking spheres per side trace a curve that
    // sweeps out and then up, which is what makes them read as bull horns
    // rather than spikes.
    for (int side = 0; side < 2; ++side) {
        const float sign = (side == 0 ? -1.0f : 1.0f);

        renderer.drawSphereAt(head + glm::vec3(sign * 0.20f, 0.14f, 0.0f),
                              glm::vec3(0.16f, 0.15f, 0.15f), horn);
        renderer.drawSphereAt(head + glm::vec3(sign * 0.31f, 0.26f, 0.0f),
                              glm::vec3(0.12f, 0.13f, 0.12f), horn);
        renderer.drawSphereAt(head + glm::vec3(sign * 0.34f, 0.38f, 0.0f),
                              glm::vec3(0.08f, 0.10f, 0.08f), horn);
    }

    // A heavy brow for the permanent scowl.
    renderer.drawSphere(
        makeTransform(head + glm::vec3(0.0f, 0.09f, 0.21f),
                      glm::vec3(0.34f, 0.07f, 0.08f)),
        muzzle);
}

//-----------------------------------------------------------------------------
// Selection frame, lock icon, Roary
//-----------------------------------------------------------------------------
void LevelSelectScene::drawSelectionFrame(int floorIndex) {
    Renderer& renderer = ctx().renderer;

    const float floorY = cfg::floorCenterY(floorIndex) - cfg::kFloorHeight * 0.5f;

    // Pulse between half and full brightness so the selection is obvious even
    // in a still screenshot.
    const float pulse = 0.55f + 0.45f
        * (0.5f + 0.5f * std::sin(elapsedTime_ * cfg::kSelectPulseSpeed));

    // Two rings hugging the tower, at the bottom and the top of the selected
    // floor. The open top floor stops at its battlements: measuring to the
    // full room height would leave the ring hanging in empty sky.
    // On the open top floor the ring clears the figures standing on the dais
    // rather than slicing through them at head height.
    const float upperRingOffset = floorPickHeight(floorIndex);

    // Must clear the floor discs, which overhang the wall - otherwise the
    // ring is swallowed by the very disc it is supposed to sit beside.
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

    // A padlock built from four boxes: the body, plus a three-sided shackle.
    renderer.drawBoxAt(center, glm::vec3(0.46f, 0.38f, 0.16f), color);

    renderer.drawBoxAt(center + glm::vec3(0.0f, 0.34f, 0.0f),
                       glm::vec3(0.30f, 0.07f, 0.10f), color);
    renderer.drawBoxAt(center + glm::vec3(-0.12f, 0.26f, 0.0f),
                       glm::vec3(0.07f, 0.20f, 0.10f), color);
    renderer.drawBoxAt(center + glm::vec3( 0.12f, 0.26f, 0.0f),
                       glm::vec3(0.07f, 0.20f, 0.10f), color);

    // Keyhole.
    renderer.drawBoxAt(center + glm::vec3(0.0f, -0.02f, 0.09f),
                       glm::vec3(0.09f, 0.14f, 0.04f), cfg::kLockedGrey);
}

void LevelSelectScene::drawRoary() {
    Renderer& renderer = ctx().renderer;

    // A gentle bob on top of the slide, so Roary never looks frozen.
    const float bob = std::sin(elapsedTime_ * cfg::kRoaryBobSpeed)
                    * cfg::kRoaryBobAmount;

    const float feetY = roaryY_ - cfg::kFloorHeight * 0.5f
                      + cfg::kSlabThickness + bob;
    const glm::vec3 feet(cfg::kRoaryOffsetX, feetY, cfg::kRoaryOffsetZ);

    // The lift platform he rides, in FIU gold.
    renderer.drawCylinderAt(feet + glm::vec3(0.0f, -0.08f, 0.0f),
                            glm::vec3(1.30f, 0.16f, 1.20f), cfg::kRoaryAccent);

    // He is built at a natural height of 1.42 units; this rescales every
    // measurement below if kRoarySize is changed.
    const float k = cfg::kRoarySize.y / 1.42f;
    const auto at = [&](float x, float y, float z) {
        return feet + glm::vec3(x * k, y * k, z * k);
    };
    const auto size = [&](float x, float y, float z) {
        return glm::vec3(x * k, y * k, z * k);
    };

    const glm::vec3 fur   = cfg::kPantherFur;
    const glm::vec3 cream = cfg::kPantherCream;
    const glm::vec3 kit   = cfg::kFiuKit;

    // Legs and navy shorts.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.13f : 0.13f);
        renderer.drawCylinderAt(at(x, 0.14f, 0.0f), size(0.15f, 0.28f, 0.15f), fur);
    }
    renderer.drawSphereAt(at(0.0f, 0.38f, 0.0f), size(0.50f, 0.28f, 0.44f), kit);

    // Navy jersey over a round chest, with white lettering.
    renderer.drawSphereAt(at(0.0f, 0.66f, 0.0f), size(0.54f, 0.50f, 0.46f), kit);
    renderer.drawSphereAt(at(0.0f, 0.70f, 0.21f), size(0.24f, 0.11f, 0.05f),
                          cfg::kFiuKitTrim);

    // Arms.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.31f : 0.31f);
        renderer.drawSphereAt(at(x, 0.64f, 0.0f), size(0.15f, 0.34f, 0.15f), fur);
    }

    // Head, muzzle and nose.
    const glm::vec3 head = at(0.0f, 1.06f, 0.0f);
    renderer.drawSphereAt(head, size(0.56f, 0.52f, 0.50f), fur);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.09f, 0.19f) * k,
                          size(0.32f, 0.22f, 0.22f), cream);
    renderer.drawSphereAt(head + glm::vec3(0.0f, -0.03f, 0.30f) * k,
                          size(0.09f, 0.07f, 0.06f), cfg::kPantherNose);

    // Ears.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.21f : 0.21f);
        renderer.drawSphereAt(head + glm::vec3(x, 0.24f, 0.0f) * k,
                              size(0.19f, 0.21f, 0.14f), fur);
        renderer.drawSphereAt(head + glm::vec3(x, 0.24f, 0.05f) * k,
                              size(0.10f, 0.12f, 0.08f), cfg::kPantherInner);
    }

    drawEyePair(renderer, head + glm::vec3(0.0f, 0.06f, 0.20f) * k,
                0.23f * k, 0.11f * k, cfg::kEyeWhite, cfg::kEyeDark);
}

//=============================================================================
// On-screen text
//=============================================================================

void LevelSelectScene::drawUi() {
    Renderer& renderer = ctx().renderer;
    TextRenderer& text = renderer.text();

    const float width = static_cast<float>(ctx().framebufferWidth);
    const float height = static_cast<float>(ctx().framebufferHeight);
    const float centerX = width * 0.5f;

    text.begin(ctx().framebufferWidth, ctx().framebufferHeight);

    // --- title ---------------------------------------------------------------
    text.drawCentered("ROARY'S ADVENTURE", centerX, px(30.0f),
                      px(cfg::kTitleScale), cfg::kTextPrimary);
    text.drawCentered("SAVE PAWLINA!", centerX, px(92.0f),
                      px(cfg::kSubtitleScale), cfg::kFiuGold);

    // --- progress counter, top right -----------------------------------------
    const std::string progressLabel =
        "FLOORS CLEARED " + std::to_string(ctx().progress.completedCount()) +
        "/" + std::to_string(cfg::kFloorCount);
    text.draw(progressLabel,
              width - px(24.0f) - TextRenderer::measureWidth(progressLabel, px(cfg::kHintScale)),
              px(24.0f), px(cfg::kHintScale), cfg::kTextDim);

    // --- small label beside each floor ---------------------------------------
    for (int floor = 0; floor < cfg::kFloorCount; ++floor) {
        const bool unlocked = ctx().progress.isUnlocked(floor);
        const bool completed = ctx().progress.isCompleted(floor);

        // Anchor the label to the right-hand edge of the floor in world space,
        // then project that point to the screen.
        glm::vec3 screenPoint(0.0f);
        // Far enough out to clear the selection ring, which sits just beyond
        // the overhanging floor discs.
        const glm::vec3 anchor(kRadius + 1.05f, cfg::floorCenterY(floor), 0.9f);
        if (!worldToScreen(anchor, renderer.viewProjection(),
                           ctx().framebufferWidth, ctx().framebufferHeight,
                           screenPoint)) {
            continue;
        }

        const float labelScale = px(cfg::kFloorLabelScale);
        const bool isSelected = (floor == selectedFloor_);

        // Selected floor's label is brighter; locked ones are red.
        glm::vec3 labelColor = cfg::kTextDim;
        if (!unlocked)        labelColor = cfg::kTextLocked;
        else if (completed)   labelColor = cfg::kTextComplete;
        else if (isSelected)  labelColor = cfg::kTextPrimary;

        const std::string label = std::string(isSelected ? "> " : "  ")
                                + cfg::kFloorInfo[floor].shortLabel;

        // One line of text is 7 font pixels tall. Stack the name just above
        // the anchor point and the state just below it, with a small gap, so
        // the two lines never collide.
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

    // --- bottom info panel ---------------------------------------------------
    const float panelHeight = px(190.0f);
    const float panelY = height - panelHeight - px(18.0f);
    const float panelX = px(50.0f);
    const float panelWidth = width - panelX * 2.0f;

    // A dark translucent slab so the text stays readable over the artwork.
    text.fillRect(panelX, panelY, panelWidth, panelHeight,
                  cfg::kBgBottom, 0.93f);

    // A gold rule along the top edge of the panel, tying it to the FIU theme.
    text.fillRect(panelX, panelY, panelWidth, px(3.0f), cfg::kFiuGold, 0.85f);

    const cfg::FloorInfo& info = cfg::kFloorInfo[selectedFloor_];
    const bool unlocked = ctx().progress.isUnlocked(selectedFloor_);
    const bool completed = ctx().progress.isCompleted(selectedFloor_);

    text.drawCentered(info.name, centerX, panelY + px(22.0f),
                      px(cfg::kFloorNameScale),
                      unlocked ? cfg::kTextPrimary : cfg::kTextDim);

    text.drawCentered(info.description, centerX, panelY + px(66.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    // --- status line ---------------------------------------------------------
    std::string statusText;
    glm::vec3 statusColor = cfg::kTextPrimary;
    float statusAlpha = 1.0f;

    if (!unlocked) {
        statusText = "LOCKED - CLEAR THE FLOOR BELOW FIRST";
        statusColor = cfg::kTextLocked;

        // Flash brightly for a moment after the player tries to enter it.
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

    // --- controls -----------------------------------------------------------
    text.drawCentered("W/S OR UP/DOWN - CHOOSE FLOOR     ENTER OR SPACE - START     ESC - QUIT",
                      centerX, panelY + px(136.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.drawCentered("MOUSE - HOVER AND CLICK A FLOOR     U - DEBUG UNLOCK ALL",
                      centerX, panelY + px(162.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
