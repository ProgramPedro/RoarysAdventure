// BossScene.cpp - Top floor, "Rival Showdown"
// Rocky loops Waiting -> Telegraph -> Charging -> Stunned -> Recover. Dodge the
// charge so he hits the wall, then touch him while he's stunned to damage him.

#include "scenes/BossScene.h"

#include "core/Angles.h"
#include "core/MascotModels.h"

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

bool BossScene::init() {
    resetFight();
    return true;
}

void BossScene::returnToMenu() {
    ctx().manager.requestScene(SceneId::LevelSelect);
}

void BossScene::handleInput(const Input& input) {
    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        returnToMenu();
        return;
    }

    if (input.wasPressed(GLFW_KEY_C)) {
        ctx().progress.markCompleted(kFloorIndex);
        returnToMenu();
        return;
    }

    glm::vec2 direction(0.0f);

    if (input.isDown(GLFW_KEY_W) || input.isDown(GLFW_KEY_UP))    direction.y -= 1.0f;
    if (input.isDown(GLFW_KEY_S) || input.isDown(GLFW_KEY_DOWN))  direction.y += 1.0f;
    if (input.isDown(GLFW_KEY_A) || input.isDown(GLFW_KEY_LEFT))  direction.x -= 1.0f;
    if (input.isDown(GLFW_KEY_D) || input.isDown(GLFW_KEY_RIGHT)) direction.x += 1.0f;

    const float length = glm::length(direction);
    moveDir_ = (length > 0.0f) ? direction / length : glm::vec2(0.0f);

    if (input.wasPressed(GLFW_KEY_SPACE)) {
        startDash();
    }
}

// A short burst that can't be steered; Roary can't be hurt while it lasts
void BossScene::startDash() {
    if (dashCooldown_ > 0.0f || dashTimer_ > 0.0f) {
        return;
    }

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

// If the position is too far from the centre, scale it back onto the edge
glm::vec3 BossScene::clampToArena(const glm::vec3& position, float radius) const {
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

    facingDeg_ = glm::degrees(std::atan2(moveDir_.x, moveDir_.y));
}

void BossScene::setRockyState(RockyState state, float duration) {
    rockyState_ = state;
    rockyTimer_ = duration;
}

// Rocky's state machine: each state runs until its timer (or the wall) moves him on
void BossScene::updateRocky(float deltaTime) {
    if (rockyState_ == RockyState::Beaten) {
        return;
    }

    rockyTimer_ = std::max(0.0f, rockyTimer_ - deltaTime);

    const float toPlayerX = playerPos_.x - rockyPos_.x;
    const float toPlayerZ = playerPos_.z - rockyPos_.z;
    const float toPlayerDeg = glm::degrees(std::atan2(toPlayerX, toPlayerZ));

    switch (rockyState_) {
    case RockyState::Waiting: {
        const float turn = std::min(cfg::kF4RockyTurnRate * deltaTime, 1.0f);
        rockyFacingDeg_ += shortestAngleDelta(rockyFacingDeg_, toPlayerDeg) * turn;

        const float distance = std::sqrt(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);
        if (distance > cfg::kF4RockyKeepDistance) {
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
        const float turn = std::min(cfg::kF4RockyTurnRate * 0.35f * deltaTime, 1.0f);
        rockyFacingDeg_ += shortestAngleDelta(rockyFacingDeg_, toPlayerDeg) * turn;

        if (rockyTimer_ <= 0.0f) {
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

        // If the clamp had to move him, he just hit the wall
        const glm::vec3 clamped = clampToArena(rockyPos_, cfg::kF4RockyRadius);
        const bool hitTheWall = (clamped.x != rockyPos_.x || clamped.z != rockyPos_.z);
        rockyPos_ = clamped;

        if (hitTheWall) {
            setRockyState(RockyState::Stunned, cfg::kF4RockyStunTime);
        }
        break;
    }

    case RockyState::Stunned:
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

// Two circles touch when the distance between them is at most their radii added
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
    playerPos_ = cfg::kF4StartPos;
    moveDir_   = glm::vec2(0.0f);
    facingDeg_ = 180.0f;

    dashTimer_    = 0.0f;
    dashCooldown_ = 0.0f;
    dashDir_      = glm::vec2(0.0f, 1.0f);

    hearts_ = cfg::kF4MaxHearts;
    playerInvulnTimer_ = 0.0f;

    rockyHealth_    = cfg::kF4RockyHealth;
    rockyPos_       = cfg::kF4RockyStartPos;
    rockyFacingDeg_ = 0.0f;
    setRockyState(RockyState::Waiting, cfg::kF4RockyWaitTime);

    outcomeTimer_ = 0.0f;
    victory_      = false;
}

void BossScene::update(float deltaTime) {
    if (outcomeTimer_ > 0.0f) {
        outcomeTimer_ = std::max(0.0f, outcomeTimer_ - deltaTime);

        if (outcomeTimer_ <= 0.0f) {
            if (victory_) {
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

    updateDash(deltaTime);
    movePlayer(deltaTime);
    updateRocky(deltaTime);
    tryPunish();
}

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

    Camera camera(cfg::kF4CameraPos, cfg::kF4CameraTarget);
    camera.setFovDegrees(cfg::kF4CameraFovDeg);
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawArena();
    drawRocky();
    drawPlayer();
    drawChargeLane();
    drawUi();
}

void BossScene::drawArena() {
    Renderer& renderer = ctx().renderer;

    const float floorSize = cfg::kF4ArenaRadius * 2.0f;
    const float rimSize   = (cfg::kF4ArenaRadius + cfg::kF4RimWidth) * 2.0f;
    const float thick     = cfg::kF4DeckThickness;

    renderer.drawCylinderAt(glm::vec3(0.0f, -thick * 0.5f - 0.01f, 0.0f),
                            glm::vec3(rimSize, thick, rimSize),
                            cfg::kF4Rim);

    renderer.drawCylinderAt(glm::vec3(0.0f, -thick * 0.5f, 0.0f),
                            glm::vec3(floorSize, thick, floorSize),
                            cfg::kF4Deck);
}

void BossScene::drawChargeLane() {
    if (rockyState_ != RockyState::Telegraph) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    const float facingRad = glm::radians(rockyFacingDeg_);
    const glm::vec3 forward(std::sin(facingRad), 0.0f, std::cos(facingRad));

    // Step forward from Rocky until the next step would leave the arena
    const float limit = cfg::kF4ArenaRadius - cfg::kF4RockyRadius;
    const float stepSize = 0.1f;
    float length = 0.0f;

    while (glm::length(rockyPos_ + forward * (length + stepSize)) <= limit) {
        length += stepSize;
    }
    length = std::max(length, 0.5f);

    const glm::vec3 centre = rockyPos_ + forward * (length * 0.5f);

    renderer.setDepthWrite(false);
    renderer.drawBox(
        makeTransform(glm::vec3(centre.x, 0.03f, centre.z),
                      glm::vec3(cfg::kF4LaneWidth, 0.02f, length),
                      glm::vec3(0.0f, rockyFacingDeg_, 0.0f)),
        cfg::kF4LaneColor, cfg::kF4LaneAlpha, 1.0f);
    renderer.setDepthWrite(true);
}

void BossScene::drawRocky() {
    if (rockyState_ == RockyState::Beaten) {
        return;
    }

    float tint = 0.0f;
    glm::vec3 tintColor(1.0f);

    if (rockyState_ == RockyState::Stunned) {
        tint = 0.55f;
        tintColor = cfg::kF4RockyStunTint;
    } else if (rockyState_ == RockyState::Recover) {
        tint = 0.40f;
        tintColor = cfg::kF4RockyHurtTint;
    }

    mascots::drawBull(ctx().renderer, rockyPos_, cfg::kF4RockyHeight,
                      rockyFacingDeg_, tint, tintColor);
}

void BossScene::drawPlayer() {
    const float tint = isInvulnerable() ? cfg::kF4InvulnTintMix : 0.0f;

    mascots::drawRoary(ctx().renderer, playerPos_,
                       mascots::kRoaryNaturalHeight * cfg::kF4PlayerScale,
                       facingDeg_, 0.0f, tint, cfg::kF4InvulnTint);
}

float BossScene::px(float designPixels) const {
    return designPixels * uiScale_;
}

void BossScene::drawUi() {
    TextRenderer& text = ctx().renderer.text();

    const int   width   = ctx().framebufferWidth;
    const int   height  = ctx().framebufferHeight;
    const float centerX = static_cast<float>(width) * 0.5f;
    const float bottom  = static_cast<float>(height);
    const float hint    = px(cfg::kHintScale);

    text.begin(width, height);

    text.drawCentered(cfg::kFloorInfo[kFloorIndex].name, centerX, px(32.0f),
                      px(cfg::kFloorNameScale), cfg::kTextPrimary);

    text.draw("HEARTS " + std::to_string(hearts_) + "/" + std::to_string(cfg::kF4MaxHearts),
              px(24.0f), px(24.0f), hint, cfg::kTextPrimary);
    text.draw("ROCKY " + std::to_string(rockyHealth_) + "/" + std::to_string(cfg::kF4RockyHealth),
              px(24.0f), px(54.0f), hint, cfg::kTextPrimary);
    text.draw(dashCooldown_ <= 0.0f ? "DASH READY" : "DASH RECHARGING",
              px(24.0f), px(84.0f), hint,
              dashCooldown_ <= 0.0f ? cfg::kTextComplete : cfg::kTextDim);

    if (outcomeTimer_ > 0.0f) {
        text.drawCentered(victory_ ? "ROCKY IS DOWN!" : "KNOCKED OUT - TRY AGAIN",
                          centerX, bottom * 0.42f, px(cfg::kFloorNameScale),
                          victory_ ? cfg::kTextComplete : cfg::kTextLocked);
    } else if (rockyState_ == RockyState::Telegraph) {
        text.drawCentered("HE IS GOING TO CHARGE - GET OUT OF THE LANE",
                          centerX, bottom - px(126.0f), hint, cfg::kTextLocked);
    } else if (rockyState_ == RockyState::Stunned) {
        text.drawCentered("HE IS DOWN - TOUCH HIM NOW",
                          centerX, bottom - px(126.0f), hint, cfg::kTextComplete);
    }

    text.drawCentered("WASD - MOVE     SPACE - DASH",
                      centerX, bottom - px(90.0f), hint, cfg::kTextPrimary);
    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f), hint, cfg::kTextDim);

    text.end();
}
