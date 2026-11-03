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

#include "core/Angles.h"        // shortestAngleDelta()
#include "core/MascotModels.h"  // mascots::drawRoary()

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

    dashTimer_      = 0.0f;
    dashCooldown_   = 0.0f;
    dashDir_        = glm::vec2(0.0f, 1.0f);
    hearts_      = cfg::kF4MaxHearts;
    elapsedTime_ = 0.0f;

    rockyHealth_  = cfg::kF4RockyHealth;
    rockyPos_     = glm::vec3(0.0f, 0.0f, -4.0f);   // waiting across the arena
    rockyFacingDeg_ = 0.0f;
    setRockyState(RockyState::Waiting, cfg::kF4RockyWaitTime);

    playerInvulnTimer_ = 0.0f;
    outcomeTimer_      = 0.0f;
    victory_           = false;
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

}

void BossScene::updateDash(float deltaTime) {
    if (dashCooldown_ > 0.0f) {
        dashCooldown_ = std::max(0.0f, dashCooldown_ - deltaTime);
    }

    if (dashTimer_ > 0.0f) {
        dashTimer_ = std::max(0.0f, dashTimer_ - deltaTime);
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

}

//=============================================================================
// Rocky the bull
//=============================================================================

void BossScene::setRockyState(RockyState state, float duration) {
    rockyState_ = state;
    rockyTimer_ = duration;
}

void BossScene::updateRocky(float deltaTime) {
    if (rockyState_ == RockyState::Beaten) {
        return;
    }

    rockyTimer_ = std::max(0.0f, rockyTimer_ - deltaTime);

    // Where Roary is, relative to Rocky. Used by most states below.
    const float toPlayerX = playerPos_.x - rockyPos_.x;
    const float toPlayerZ = playerPos_.z - rockyPos_.z;
    const float toPlayerDeg = glm::degrees(std::atan2(toPlayerX, toPlayerZ));

    switch (rockyState_) {
    case RockyState::Waiting: {
        // Square up, slowly. The slow turn is deliberate: it lets the player
        // circle him, and it means his facing at the end of the telegraph is
        // something they could have predicted.
        const float turn = std::min(cfg::kF4RockyTurnRate * deltaTime, 1.0f);
        rockyFacingDeg_ += shortestAngleDelta(rockyFacingDeg_, toPlayerDeg) * turn;

        // Shuffle toward Roary so he cannot be stalled out at the far rim.
        const float distance = std::sqrt(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);
        if (distance > 4.0f) {
            rockyPos_.x += (toPlayerX / distance) * cfg::kF4RockyWalkSpeed * deltaTime;
            rockyPos_.z += (toPlayerZ / distance) * cfg::kF4RockyWalkSpeed * deltaTime;
            rockyPos_ = clampToArena(rockyPos_, cfg::kF4RockyRadius);
        }

        if (rockyTimer_ <= 0.0f) {
            setRockyState(RockyState::Telegraph, cfg::kF4RockyTelegraphTime);
        }
        break;
    }

    case RockyState::Telegraph: {
        // Keep tracking, but far more slowly, so the lane drifts only a little
        // once it is up. Locking it completely at the START would let the
        // player walk out before the lane even appeared.
        const float turn = std::min(cfg::kF4RockyTurnRate * 0.35f * deltaTime, 1.0f);
        rockyFacingDeg_ += shortestAngleDelta(rockyFacingDeg_, toPlayerDeg) * turn;

        if (rockyTimer_ <= 0.0f) {
            // Commit. From here he cannot steer, which is the whole reason
            // stepping aside works.
            const float facingRad = glm::radians(rockyFacingDeg_);
            chargeDir_ = glm::vec2(std::sin(facingRad), std::cos(facingRad));
            setRockyState(RockyState::Charging, 0.0f);
        }
        break;
    }

    case RockyState::Charging: {
        rockyPos_.x += chargeDir_.x * cfg::kF4RockyChargeSpeed * deltaTime;
        rockyPos_.z += chargeDir_.y * cfg::kF4RockyChargeSpeed * deltaTime;

        checkChargeContact();

        // Did he run out of arena? The clamp tells us: if it moved him, he was
        // trying to leave, which means he just met the parapet.
        const glm::vec3 clamped = clampToArena(rockyPos_, cfg::kF4RockyRadius);
        const bool hitTheWall = (clamped.x != rockyPos_.x || clamped.z != rockyPos_.z);
        rockyPos_ = clamped;

        if (hitTheWall) {
            setRockyState(RockyState::Stunned, cfg::kF4RockyStunTime);
        }
        break;
    }

    case RockyState::Stunned:
        // Wide open. tryPunish() is what ends this early.
        if (rockyTimer_ <= 0.0f) {
            setRockyState(RockyState::Recover, cfg::kF4RockyRecoverTime);
        }
        break;

    case RockyState::Recover:
        if (rockyTimer_ <= 0.0f) {
            setRockyState(RockyState::Waiting, cfg::kF4RockyWaitTime);
        }
        break;

    case RockyState::Beaten:
        break;
    }
}

void BossScene::checkChargeContact() {
    if (isInvulnerable()) {
        return;
    }

    const float dx = playerPos_.x - rockyPos_.x;
    const float dz = playerPos_.z - rockyPos_.z;
    const float reach = cfg::kF4RockyRadius + cfg::kF4PlayerRadius;

    if (dx * dx + dz * dz <= reach * reach) {
        takeHit();
    }
}

void BossScene::tryPunish() {
    // Only while he is reeling against the parapet. At any other moment
    // touching him does nothing - he is a wall, not a target.
    if (rockyState_ != RockyState::Stunned) {
        return;
    }

    const float dx = playerPos_.x - rockyPos_.x;
    const float dz = playerPos_.z - rockyPos_.z;
    const float reach = cfg::kF4RockyPunishRange + cfg::kF4PlayerRadius;

    if (dx * dx + dz * dz > reach * reach) {
        return;
    }

    rockyHealth_ = std::max(0, rockyHealth_ - 1);

    if (rockyHealth_ <= 0) {
        setRockyState(RockyState::Beaten, 0.0f);
        victory_      = true;
        outcomeTimer_ = cfg::kF4VictoryTime;
    } else {
        // Knocked back into his recovery, so one stun cannot be milked for
        // several hits in a row.
        setRockyState(RockyState::Recover, cfg::kF4RockyRecoverTime);
    }
}

void BossScene::takeHit() {
    hearts_ = std::max(0, hearts_ - 1);
    playerInvulnTimer_ = cfg::kF4PlayerHitInvuln;

    if (hearts_ <= 0) {
        victory_      = false;
        outcomeTimer_ = cfg::kF4DefeatTime;
    }
}

void BossScene::resetFight() {
    playerPos_ = clampToArena(cfg::kF4StartPos, cfg::kF4PlayerRadius);
    moveDir_   = glm::vec2(0.0f);
    facingDeg_ = 180.0f;

    dashTimer_    = 0.0f;
    dashCooldown_ = 0.0f;

    hearts_ = cfg::kF4MaxHearts;
    playerInvulnTimer_ = 0.0f;

    // Rocky goes back to full health too. The fight is short, so restarting it
    // whole is kinder than it sounds - and a boss who stays damaged after
    // beating you makes the loss meaningless.
    rockyHealth_    = cfg::kF4RockyHealth;
    rockyPos_       = glm::vec3(0.0f, 0.0f, -4.0f);
    rockyFacingDeg_ = 0.0f;
    setRockyState(RockyState::Waiting, cfg::kF4RockyWaitTime);

    outcomeTimer_ = 0.0f;
    victory_      = false;
}

void BossScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    // A win or a loss banner is up: the fight is frozen while it reads, then
    // the level reacts. Returning early is what freezes it.
    if (outcomeTimer_ > 0.0f) {
        outcomeTimer_ = std::max(0.0f, outcomeTimer_ - deltaTime);

        if (outcomeTimer_ <= 0.0f) {
            if (victory_) {
                // The real win condition at last: floor 4 complete.
                ctx().progress.markCompleted(kFloorIndex);
                returnToMenu();
            } else {
                resetFight();
            }
        }
        return;
    }

    if (playerInvulnTimer_ > 0.0f) {
        playerInvulnTimer_ = std::max(0.0f, playerInvulnTimer_ - deltaTime);
    }

    // 1. Dash timers first, so movePlayer() sees this frame's dash state
    //    rather than the previous frame's.
    updateDash(deltaTime);

    // 2. Move Roary. Everything below wants his final position this frame.
    movePlayer(deltaTime);

    // 3. Rocky acts, which includes checking whether his charge caught Roary.
    updateRocky(deltaTime);

    // 4. And last, whether Roary reached him while he is down. After Rocky
    //    moves, or a punish could land on a bull who has already got up.
    tryPunish();
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
    drawChargeLane();   // on the deck, so under everything standing on it
    drawRocky();
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

void BossScene::drawChargeLane() {
    // Only while he is winding up. Once he is moving, the bull himself is the
    // warning and a lane on the floor just adds clutter.
    if (rockyState_ != RockyState::Telegraph) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    const float facingRad = glm::radians(rockyFacingDeg_);
    const glm::vec3 forward(std::sin(facingRad), 0.0f, std::cos(facingRad));

    // How far he will actually travel: from where he stands to the point where
    // the parapet stops him. That is a ray meeting a circle, and because he
    // starts INSIDE the circle there is exactly one answer in front of him.
    //
    // A point on his path is  P + D*t.  We want the t where it is exactly
    // `limit` from the middle of the arena, so solve |P + D*t| = limit:
    //
    //     (P + D*t) . (P + D*t) = limit^2
    //     t^2 + 2(P.D)t + (P.P - limit^2) = 0        (D is a unit vector)
    //
    // which is the quadratic formula with a = 1. Taking the + root gives the
    // crossing ahead of him rather than the one behind.
    const float limit = cfg::kF4ArenaRadius - cfg::kF4RockyRadius;
    const float pDotD = rockyPos_.x * forward.x + rockyPos_.z * forward.z;
    const float pDotP = rockyPos_.x * rockyPos_.x + rockyPos_.z * rockyPos_.z;

    const float discriminant = pDotD * pDotD - (pDotP - limit * limit);
    const float length = std::max(0.5f,
        -pDotD + std::sqrt(std::max(0.0f, discriminant)));

    const glm::vec3 centre = rockyPos_ + forward * (length * 0.5f);

    // How far through the wind-up he is, so the lane brightens as the moment
    // approaches. This is the one piece of timing the player reads, so it is
    // worth making it legible.
    const float progress = (cfg::kF4RockyTelegraphTime > 0.0f)
        ? 1.0f - (rockyTimer_ / cfg::kF4RockyTelegraphTime)
        : 1.0f;

    renderer.setDepthWrite(false);
    renderer.drawBox(
        makeTransform(glm::vec3(centre.x, 0.03f, centre.z),
                      glm::vec3(cfg::kF4LaneWidth, 0.02f, length),
                      glm::vec3(0.0f, rockyFacingDeg_, 0.0f)),
        cfg::kF4LaneColor,
        cfg::kF4LaneAlpha * (0.45f + progress * 0.55f),
        1.0f);
    renderer.setDepthWrite(true);
}

void BossScene::drawRocky() {
    Renderer& renderer = ctx().renderer;

    if (rockyState_ == RockyState::Beaten) {
        return;   // he is down; nothing left to draw
    }

    // Blob shadow, same as Roary gets, scaled to how much bigger he is.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(rockyPos_.x, 0.015f, rockyPos_.z),
                            glm::vec3(cfg::kF4RockyRadius * 2.4f, 0.02f,
                                      cfg::kF4RockyRadius * 2.4f),
                            cfg::kF3ShadowColor, cfg::kF3ShadowAlpha, 1.0f);
    renderer.setDepthWrite(true);

    // The tint is his state readout, and it is the only one he has: orange
    // while he is reeling and open to a hit, white for the brief recovery
    // after he takes one, plain grey the rest of the time. No animation.
    float tint = 0.0f;
    glm::vec3 tintColor(1.0f);

    if (rockyState_ == RockyState::Stunned) {
        tint = 0.55f;
        tintColor = cfg::kF4RockyStunTint;
    } else if (rockyState_ == RockyState::Recover) {
        tint = 0.40f;
        tintColor = cfg::kF4RockyHurtTint;
    }

    mascots::drawBull(renderer, rockyPos_, cfg::kF4RockyHeight,
                      rockyFacingDeg_, tint, tintColor);
}

void BossScene::drawPlayer() {
    Renderer& renderer = ctx().renderer;

    // Blob shadow, so he is not floating over the deck.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(playerPos_.x, 0.015f, playerPos_.z),
                            glm::vec3(cfg::kF4PlayerRadius * 2.3f, 0.02f,
                                      cfg::kF4PlayerRadius * 2.3f),
                            cfg::kF3ShadowColor, cfg::kF3ShadowAlpha, 1.0f);
    renderer.setDepthWrite(true);

    // The shared model from core/MascotModels.cpp, the same one floors 1 and 2
    // use. A static pose - no walk animation, and no ghost trail behind a
    // dash. Those are deliberately on hold.
    //
    // The ONE thing still varying is the tint: he washes pale blue while the
    // dash's invincibility frames are up. That is not decoration, it is the
    // only way the player can tell the dash is currently protecting them, so
    // it stays even with the animation stripped out.
    const float tint = isInvulnerable() ? cfg::kF4InvulnTintMix : 0.0f;

    ::mascots::drawRoary(renderer, playerPos_,
                         mascots::kRoaryNaturalHeight * cfg::kF4PlayerScale,
                         facingDeg_, 0.0f, tint, cfg::kF4InvulnTint);
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

    // --- Rocky's health, top centre ------------------------------------------
    // Under the title, where a boss bar belongs. Drawn as one bar split into
    // segments so a hit is a visible chunk disappearing rather than a slow
    // slide - with only three hits, each one should feel like an event.
    if (rockyState_ != RockyState::Beaten) {
        const float barWidth  = px(cfg::kF4BossBarWidth);
        const float barHeight = px(14.0f);
        const float barX      = centerX - barWidth * 0.5f;
        const float barY      = px(112.0f);

        text.fillRect(barX - px(3.0f), barY - px(3.0f),
                      barWidth + px(6.0f), barHeight + px(6.0f),
                      cfg::kF4BossBarTrack, 0.85f);

        const float gap = px(4.0f);
        const float segment =
            (barWidth - gap * static_cast<float>(cfg::kF4RockyHealth - 1))
            / static_cast<float>(cfg::kF4RockyHealth);

        for (int i = 0; i < cfg::kF4RockyHealth; ++i) {
            if (i >= rockyHealth_) {
                continue;
            }
            text.fillRect(barX + static_cast<float>(i) * (segment + gap), barY,
                          segment, barHeight, cfg::kF4BossBarFill, 0.95f);
        }

        text.drawCentered("ROCKY", centerX, barY + barHeight + px(6.0f),
                          px(cfg::kHintScale), cfg::kBullHorn);
    }

    // --- what Rocky is doing, so the player can learn the pattern ------------
    const char* rockyLabel = nullptr;
    glm::vec3 rockyLabelColor = cfg::kTextDim;

    if (rockyState_ == RockyState::Telegraph) {
        rockyLabel = "HE IS GOING TO CHARGE - GET OUT OF THE LANE";
        rockyLabelColor = cfg::kF4LaneColor;
    } else if (rockyState_ == RockyState::Stunned) {
        rockyLabel = "HE IS DOWN - TOUCH HIM NOW";
        rockyLabelColor = cfg::kF4DashReady;
    }

    if (rockyLabel != nullptr && outcomeTimer_ <= 0.0f) {
        text.drawCentered(rockyLabel, centerX, bottom - px(160.0f),
                          px(cfg::kHintScale), rockyLabelColor);
    }

    // --- win / lose banner ---------------------------------------------------
    if (outcomeTimer_ > 0.0f) {
        const std::string banner = victory_ ? "ROCKY IS DOWN!"
                                            : "KNOCKED OUT - TRY AGAIN";
        const float scale = px(cfg::kFloorNameScale * 0.85f);
        const float textWidth = TextRenderer::measureWidth(banner, scale);
        const float bannerY = static_cast<float>(height) * 0.42f;

        text.fillRect(centerX - textWidth * 0.5f - px(18.0f), bannerY - px(12.0f),
                      textWidth + px(36.0f),
                      TextRenderer::measureHeight(scale) + px(24.0f),
                      cfg::kBgBottom, 0.82f);

        text.drawCentered(banner, centerX, bannerY, scale,
                          victory_ ? cfg::kTextComplete : cfg::kF4HeartFull);
    }

    // --- the usual hints -----------------------------------------------------

    text.drawCentered("WASD - MOVE     SPACE - DASH",
                      centerX, bottom - px(90.0f),
                      px(cfg::kHintScale), cfg::kTextPrimary);

    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
