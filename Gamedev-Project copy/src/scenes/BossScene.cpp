//=============================================================================
// BossScene.cpp - THE TOP FLOOR, "RIVAL SHOWDOWN"
//
// Read StealthScene.cpp's header first if you have not: the frame order
// (handleInput -> update -> render), the deltaTime rule and the facing-angle
// convention are all the same here, and they are explained there in full.
//
// WHAT IS DIFFERENT UP HERE
//
//   * The arena is a CIRCLE, not a rectangle, and it is empty. Collision is
//     one distance check. There is no pathfinding because there is nothing to
//     walk around.
//   * Roary can DASH. That is the only new verb, and the whole fight is built
//     around it.
//   * He has hearts instead of a suspicion meter, because up here things hit
//     you rather than notice you.
//=============================================================================

#include "scenes/BossScene.h"

#include "scenes/Characters.h"   // drawRoary() and shortestAngleDelta()

#include "core/Camera.h"
#include "core/GameProgress.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/SceneContext.h"
#include "core/SceneManager.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace {

// Same aspect-ratio fix floor 3 uses. A window narrower than the design shape
// would otherwise crop the sides of the arena off; widening the field of view
// instead trades that for empty space above and below.
float fovForAspect(float baseFovDeg, float aspect) {
    if (aspect >= cfg::kF3DesignAspect || aspect <= 0.0f) {
        return baseFovDeg;
    }

    const float halfWidth =
        std::tan(glm::radians(baseFovDeg * 0.5f)) * cfg::kF3DesignAspect;

    return glm::degrees(2.0f * std::atan(halfWidth / aspect));
}

// A point on a circle of `radius` at `degrees` measured from +Z toward +X.
// Same convention as the facing angles, so an angle means the same thing
// whether it is pointing a character or placing a block of the parapet.
glm::vec3 pointOnCircle(float degrees, float radius, float y) {
    const float radians = glm::radians(degrees);
    return glm::vec3(std::sin(radians) * radius, y, std::cos(radians) * radius);
}

} // namespace

bool BossScene::init() {
    playerPos_   = clampToArena(cfg::kF4StartPos, cfg::kF4PlayerRadius);
    moveDir_     = glm::vec2(0.0f);
    // 180 degrees looks down -Z, which is AWAY from the camera and into the
    // middle of the arena where the mascots will be. 0 would face the camera.
    facingDeg_   = 180.0f;
    walkCycle_   = 0.0f;

    dashTimer_      = 0.0f;
    dashCooldown_   = 0.0f;
    dashDir_        = glm::vec2(0.0f, 1.0f);
    dashTrailCount_ = 0;
    dashTrailTimer_ = 0.0f;

    hearts_      = cfg::kF4MaxHearts;
    elapsedTime_ = 0.0f;
    return true;
}

void BossScene::returnToMenu() {
    ctx().manager.requestScene(SceneId::LevelSelect);
}

//=============================================================================
// Input
//=============================================================================

void BossScene::handleInput(const Input& input) {
    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        returnToMenu();
        return;
    }

    // DEBUG, until beating the three mascots is the real win condition.
    if (input.wasPressed(GLFW_KEY_C)) {
        ctx().progress.markCompleted(kFloorIndex);
        returnToMenu();
        return;
    }

    // Movement is isDown(), because we want it every frame the key is held.
    glm::vec2 direction(0.0f);

    if (input.isDown(GLFW_KEY_W) || input.isDown(GLFW_KEY_UP))    direction.y -= 1.0f;
    if (input.isDown(GLFW_KEY_S) || input.isDown(GLFW_KEY_DOWN))  direction.y += 1.0f;
    if (input.isDown(GLFW_KEY_A) || input.isDown(GLFW_KEY_LEFT))  direction.x -= 1.0f;
    if (input.isDown(GLFW_KEY_D) || input.isDown(GLFW_KEY_RIGHT)) direction.x += 1.0f;

    // Normalize, or holding two keys would move him 1.41x faster diagonally.
    const float length = glm::length(direction);
    moveDir_ = (length > 0.0f) ? direction / length : glm::vec2(0.0f);

    // The dash is wasPressed(), not isDown(): holding space must not chain
    // dashes the instant the cooldown expires. Each dash is a decision.
    if (input.wasPressed(GLFW_KEY_SPACE)) {
        startDash();
    }
}

//=============================================================================
// The dash
//=============================================================================

void BossScene::startDash() {
    // Pressing space when it is not available does nothing at all. No queueing
    // the press for later: a dash that fires half a second after you asked for
    // it is worse than one that does not fire, because you have already moved
    // on and it throws you somewhere you did not choose.
    if (dashCooldown_ > 0.0f || dashTimer_ > 0.0f) {
        return;
    }

    // Dash the way he is MOVING, or - if he is standing still - the way he is
    // looking. Without the second case, a dash from a standstill would go
    // nowhere, which feels broken rather than deliberate.
    if (moveDir_ != glm::vec2(0.0f)) {
        dashDir_ = moveDir_;
    } else {
        const float facingRad = glm::radians(facingDeg_);
        dashDir_ = glm::vec2(std::sin(facingRad), std::cos(facingRad));
    }

    dashTimer_    = cfg::kF4DashDuration;
    dashCooldown_ = cfg::kF4DashCooldown;

    // Start the trail empty so ghosts from an earlier dash do not appear
    // strung out behind this one.
    dashTrailCount_ = 0;
    dashTrailTimer_ = 0.0f;
}

void BossScene::updateDash(float deltaTime) {
    if (dashCooldown_ > 0.0f) {
        dashCooldown_ = std::max(0.0f, dashCooldown_ - deltaTime);
    }

    if (dashTimer_ <= 0.0f) {
        return;
    }

    dashTimer_ = std::max(0.0f, dashTimer_ - deltaTime);

    // Drop a ghost every so often rather than every frame, or the trail is a
    // solid smear at 60fps and a dotted line at 20.
    dashTrailTimer_ -= deltaTime;

    if (dashTrailTimer_ <= 0.0f) {
        dashTrailTimer_ = cfg::kF4DashDuration
                        / static_cast<float>(cfg::kF4DashTrailCount);

        // Shuffle everything one slot older, then write the newest at the
        // front. With four entries this is cheaper than any clever structure.
        for (int i = cfg::kF4DashTrailCount - 1; i > 0; --i) {
            dashTrail_[static_cast<std::size_t>(i)] =
                dashTrail_[static_cast<std::size_t>(i - 1)];
        }

        dashTrail_[0] = playerPos_;
        dashTrailCount_ = std::min(dashTrailCount_ + 1, cfg::kF4DashTrailCount);
    }
}

float BossScene::dashCharge() const {
    if (cfg::kF4DashCooldown <= 0.0f) {
        return 1.0f;
    }

    // Counting DOWN from the cooldown, so turn it into a 0..1 that counts up.
    return 1.0f - std::clamp(dashCooldown_ / cfg::kF4DashCooldown, 0.0f, 1.0f);
}

//=============================================================================
// Simulation
//=============================================================================

glm::vec3 BossScene::clampToArena(const glm::vec3& position, float radius) const {
    // The entire collision system for this level.
    //
    // Measure how far out from the middle he is; if that is further than the
    // arena allows, scale the vector back down until it is not. Dividing by
    // `distance` turns it into a unit direction, and multiplying by `limit`
    // puts him exactly on the rim.
    const float limit = cfg::kF4ArenaRadius - radius;
    const float distance = std::sqrt(position.x * position.x +
                                     position.z * position.z);

    if (distance <= limit || distance < 1e-5f) {
        return position;
    }

    const float scale = limit / distance;
    return glm::vec3(position.x * scale, position.y, position.z * scale);
}

void BossScene::movePlayer(float deltaTime) {
    // Dashing overrides the controls completely. Once it is committed the
    // player cannot steer it, which is what makes it a decision with a cost
    // rather than just "move faster for a moment".
    if (dashTimer_ > 0.0f) {
        playerPos_ += glm::vec3(dashDir_.x, 0.0f, dashDir_.y)
                    * cfg::kF4DashSpeed * deltaTime;
        playerPos_ = clampToArena(playerPos_, cfg::kF4PlayerRadius);

        facingDeg_ = glm::degrees(std::atan2(dashDir_.x, dashDir_.y));
        walkCycle_ += cfg::kF4BobSpeed * deltaTime;
        return;
    }

    if (moveDir_ == glm::vec2(0.0f)) {
        return;
    }

    playerPos_ += glm::vec3(moveDir_.x, 0.0f, moveDir_.y)
                * cfg::kF4WalkSpeed * deltaTime;
    playerPos_ = clampToArena(playerPos_, cfg::kF4PlayerRadius);

    // Face where he is heading, easing rather than snapping. The 0..1 clamp
    // stops a long frame from overshooting and jittering around the target.
    const float targetDeg = glm::degrees(std::atan2(moveDir_.x, moveDir_.y));
    const float turn = std::min(cfg::kF4TurnSpeed * deltaTime, 1.0f);
    facingDeg_ += shortestAngleDelta(facingDeg_, targetDeg) * turn;

    walkCycle_ += cfg::kF4BobSpeed * deltaTime;
}

void BossScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    // Dash timers first, so movePlayer() sees this frame's dash state rather
    // than the previous frame's.
    updateDash(deltaTime);
    movePlayer(deltaTime);
}

//=============================================================================
// Drawing
//=============================================================================

void BossScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width  = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    renderer.drawBackground(
        glm::mix(cfg::kBgTop, cfg::kF4Wall, 0.55f),
        glm::mix(cfg::kBgBottom, cfg::kF4Wall, 0.25f),
        cfg::kFiuGold,
        glm::vec2(0.0f, 0.25f),
        cfg::kBgGlowRadius,
        0.30f);

    const float aspect = static_cast<float>(width)
                       / static_cast<float>(std::max(height, 1));

    Camera camera(cfg::kF4CameraPos, cfg::kF4CameraTarget);
    camera.setFovDegrees(fovForAspect(cfg::kF4CameraFovDeg, aspect));
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawArena();
    drawPlayer();
    drawUi();
}

void BossScene::drawArena() {
    Renderer& renderer = ctx().renderer;

    // --- the deck ------------------------------------------------------------
    // Concentric discs, largest first, each one drawn slightly higher than the
    // last so it wins the depth test. The alternating colours read as rings,
    // and the rings are what tell you how close to the edge you are.
    for (int ring = cfg::kF4DeckRings; ring >= 1; --ring) {
        const float fraction = static_cast<float>(ring)
                             / static_cast<float>(cfg::kF4DeckRings);
        const float radius = cfg::kF4ArenaRadius * fraction;

        const glm::vec3 color = (ring % 2 == 0) ? cfg::kF4DeckA : cfg::kF4DeckB;

        renderer.drawCylinderAt(
            glm::vec3(0.0f,
                      -cfg::kF4DeckThickness * 0.5f
                          + static_cast<float>(cfg::kF4DeckRings - ring) * 0.004f,
                      0.0f),
            glm::vec3(radius * 2.0f, cfg::kF4DeckThickness, radius * 2.0f),
            color);
    }

    // --- the parapet ---------------------------------------------------------
    // A ring of boxes stepped around the circle. Each one is rotated to the
    // angle it sits at so its face points outward, and made slightly wider
    // than its share of the circumference so neighbours overlap and no gaps
    // show between them.
    const float step = 360.0f / static_cast<float>(cfg::kF4ParapetSegs);
    const float segWidth =
        2.0f * cfg::kF4ArenaRadius
        * std::sin(glm::radians(step * 0.5f)) * 1.15f;

    for (int i = 0; i < cfg::kF4ParapetSegs; ++i) {
        const float angle = step * static_cast<float>(i);

        renderer.drawBox(
            makeTransform(
                pointOnCircle(angle,
                              cfg::kF4ArenaRadius + cfg::kF4ParapetThick * 0.5f,
                              cfg::kF4ParapetHeight * 0.5f),
                glm::vec3(segWidth, cfg::kF4ParapetHeight, cfg::kF4ParapetThick),
                glm::vec3(0.0f, angle, 0.0f)),
            cfg::kF4Wall);
    }

    // --- the battlements -----------------------------------------------------
    // Blocks spaced around the top of the parapet, with gaps between them.
    // Fewer of these than parapet segments, which is what produces the gaps.
    const float merlonStep = 360.0f / static_cast<float>(cfg::kF4MerlonCount);

    for (int i = 0; i < cfg::kF4MerlonCount; ++i) {
        const float angle = merlonStep * static_cast<float>(i);

        renderer.drawBox(
            makeTransform(
                pointOnCircle(angle,
                              cfg::kF4ArenaRadius + cfg::kF4ParapetThick * 0.5f,
                              cfg::kF4ParapetHeight + cfg::kF4MerlonHeight * 0.5f),
                glm::vec3(segWidth * 1.4f, cfg::kF4MerlonHeight,
                          cfg::kF4ParapetThick * 1.05f),
                glm::vec3(0.0f, angle, 0.0f)),
            cfg::kF4Rim);
    }
}

void BossScene::drawPlayer() {
    Renderer& renderer = ctx().renderer;

    // --- the dash trail ------------------------------------------------------
    // Ghost copies of Roary at the positions he passed through, oldest faintest.
    // Drawn before him so he is always on top of his own afterimages.
    //
    // There is no alpha on drawRoary(), so the "fade" is done by washing each
    // ghost further toward the arena's background colour - older ghosts blend
    // in more, which reads the same way at this distance.
    if (dashTimer_ > 0.0f) {
        for (int i = 0; i < dashTrailCount_; ++i) {
            const float age = static_cast<float>(i + 1)
                            / static_cast<float>(cfg::kF4DashTrailCount + 1);

            ::drawRoary(renderer, dashTrail_[static_cast<std::size_t>(i)],
                        facingDeg_, walkCycle_, false,
                        cfg::kF4PlayerScale * (1.0f - age * 0.25f),
                        0.55f + age * 0.35f, cfg::kF4DeckB);
        }
    }

    // --- Roary himself -------------------------------------------------------
    // Washed pale blue while the invincibility frames are up. That tint is the
    // ONLY thing telling the player the dash is currently protecting them, so
    // it needs to be unmistakable.
    const float tint = isInvulnerable() ? cfg::kF4InvulnTintMix : 0.0f;

    ::drawRoary(renderer, playerPos_, facingDeg_, walkCycle_,
                moveDir_ != glm::vec2(0.0f) || dashTimer_ > 0.0f,
                cfg::kF4PlayerScale, tint, cfg::kF4InvulnTint);
}

float BossScene::px(float designPixels) const {
    return designPixels * uiScale_;
}

void BossScene::drawUi() {
    TextRenderer& text = ctx().renderer.text();

    const int   width  = ctx().framebufferWidth;
    const int   height = ctx().framebufferHeight;
    const float fWidth = static_cast<float>(width);
    const float bottom = static_cast<float>(height);
    const float centerX = fWidth * 0.5f;

    text.begin(width, height);

    const cfg::FloorInfo& info = cfg::kFloorInfo[kFloorIndex];

    text.drawCentered(info.name, centerX, px(32.0f),
                      px(cfg::kFloorNameScale), cfg::kTextPrimary);
    text.drawCentered(info.description, centerX, px(80.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    // --- hearts, top left ----------------------------------------------------
    // Plain squares: the built-in font is uppercase letters only, so there is
    // no heart glyph to print. Shape matters less than the count being obvious.
    const float heartSize = px(cfg::kF4HeartSize);
    const float heartGap  = px(cfg::kF4HeartGap);

    for (int i = 0; i < cfg::kF4MaxHearts; ++i) {
        const bool full = (i < hearts_);

        text.fillRect(px(24.0f) + static_cast<float>(i) * (heartSize + heartGap),
                      px(26.0f), heartSize, heartSize,
                      full ? cfg::kF4HeartFull : cfg::kF4HeartEmpty,
                      full ? 0.95f : 0.55f);
    }

    // --- dash readiness, under the hearts ------------------------------------
    // A bar that refills over the cooldown. Showing the recharge rather than
    // just "ready / not ready" lets the player time a dash to the moment it
    // comes back, which is the whole rhythm of the fight.
    const float pipWidth  = px(cfg::kF4DashPipWidth);
    const float pipHeight = px(10.0f);
    const float pipY      = px(26.0f) + heartSize + px(10.0f);
    const float charge    = dashCharge();
    const bool  ready     = (charge >= 1.0f);

    text.fillRect(px(24.0f) - px(2.0f), pipY - px(2.0f),
                  pipWidth + px(4.0f), pipHeight + px(4.0f),
                  cfg::kF4DashCharging, 0.70f);

    text.fillRect(px(24.0f), pipY, pipWidth * charge, pipHeight,
                  ready ? cfg::kF4DashReady : cfg::kF4HeartFull, 0.95f);

    text.draw(ready ? "DASH READY" : "DASH...",
              px(24.0f), pipY + pipHeight + px(6.0f),
              px(cfg::kHintScale),
              ready ? cfg::kF4DashReady : cfg::kTextDim);

    // --- the usual hints -----------------------------------------------------
    text.drawCentered("NO BOSSES YET - ARENA AND DASH ONLY",
                      centerX, bottom - px(126.0f),
                      px(cfg::kHintScale), cfg::kFiuGold);

    text.drawCentered("WASD - MOVE     SPACE - DASH",
                      centerX, bottom - px(90.0f),
                      px(cfg::kHintScale), cfg::kTextPrimary);

    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
