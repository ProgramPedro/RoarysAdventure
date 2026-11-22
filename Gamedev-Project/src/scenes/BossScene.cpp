// Runs the boss fight. Rocky cycles through waiting, telegraphing, charging,
// stunned and recovering, so his charge has to be dodged rather than blocked.
// Dashing carries Roary clear and makes him briefly invulnerable, on a cooldown,
// and touching Rocky while he is stunned takes health off him. The scene shows a
// win or lose message once either side runs out.
//
// Dashing past Rocky while he is charging parries him, and the next punish then
// takes two health off instead of one. Because the dash is also the way out of a
// charge, going for a parry means spending the escape to earn the bonus.
//
// Once Rocky is down to half health Sebastian joins and the rest of the fight
// has both of them going at once: Rocky charging along the ground and Sebastian
// slamming down from above. Sebastian cannot be hurt - Rocky is the ring leader
// and beating him ends the floor - so the bird is purely something to dodge.

#include "scenes/BossScene.h"

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
        rockyFacingDeg_ = toPlayerDeg;

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

        // Same moment, opposite outcome: the contact test above is Roary being
        // caught by the charge, this one is him dashing through it on purpose.
        checkParry();

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

void BossScene::setIbisState(IbisState state, float duration) {
    ibisState_ = state;
    ibisTimer_ = duration;
}

void BossScene::updateIbis(float deltaTime) {
    // He waits in the wings until Rocky is worn down to half.
    if (ibisState_ == IbisState::Away) {
        if (rockyHealth_ <= cfg::kF4IbisJoinHealth && rockyHealth_ > 0) {
            // Drop in above Roary so his arrival is impossible to miss, then
            // start on a hover rather than attacking immediately - landing on
            // somebody the instant you appear is not a fight, it is an ambush.
            ibisPos_ = glm::vec3(playerPos_.x, cfg::kF4IbisHoverY, playerPos_.z);
            ibisEntryTextTimer_ = cfg::kF4IbisEntryTextTime;
            setIbisState(IbisState::Hover, cfg::kF4IbisHoverTime);
        }
        return;
    }

    ibisTimer_ = std::max(0.0f, ibisTimer_ - deltaTime);
    ibisEntryTextTimer_ = std::max(0.0f, ibisEntryTextTimer_ - deltaTime);

    const float toPlayerX = playerPos_.x - ibisPos_.x;
    const float toPlayerZ = playerPos_.z - ibisPos_.z;
    const float flatDistance = std::sqrt(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);

    if (flatDistance > 0.01f) {
        ibisFacingDeg_ = glm::degrees(std::atan2(toPlayerX, toPlayerZ));
    }

    switch (ibisState_) {
    case IbisState::Hover: {
        // Drift after Roary, slower than he can run. Being followed rather
        // than tracked perfectly is what lets the player get out from under
        // him before the mark even appears.
        if (flatDistance > 0.1f) {
            const float step = std::min(cfg::kF4IbisDriftSpeed * deltaTime, flatDistance);
            ibisPos_.x += (toPlayerX / flatDistance) * step;
            ibisPos_.z += (toPlayerZ / flatDistance) * step;
        }

        // Climb back to hover height if he is still low from the last slam.
        ibisPos_.y = std::min(cfg::kF4IbisHoverY,
                              ibisPos_.y + cfg::kF4IbisRiseSpeed * deltaTime);

        if (ibisTimer_ <= 0.0f && ibisPos_.y >= cfg::kF4IbisHoverY - 0.05f) {
            // Snapshot. From here the circle stays put, so walking out of it
            // always works - which is the whole dodge.
            ibisMarkPos_ = glm::vec3(playerPos_.x, 0.0f, playerPos_.z);
            setIbisState(IbisState::Marking, cfg::kF4IbisMarkTime);
        }
        break;
    }

    case IbisState::Marking: {
        // Slide over the marked spot while the warning is up, so he is
        // directly above it when he drops.
        const float dx = ibisMarkPos_.x - ibisPos_.x;
        const float dz = ibisMarkPos_.z - ibisPos_.z;
        const float distance = std::sqrt(dx * dx + dz * dz);

        if (distance > 0.01f) {
            const float step = std::min(cfg::kF4IbisDriftSpeed * 2.0f * deltaTime,
                                        distance);
            ibisPos_.x += (dx / distance) * step;
            ibisPos_.z += (dz / distance) * step;
        }

        if (ibisTimer_ <= 0.0f) {
            setIbisState(IbisState::Slamming, 0.0f);
        }
        break;
    }

    case IbisState::Slamming: {
        ibisPos_.y -= cfg::kF4IbisSlamSpeed * deltaTime;

        if (ibisPos_.y <= 0.0f) {
            ibisPos_.y = 0.0f;
            checkSlamContact();
            setIbisState(IbisState::Recover, cfg::kF4IbisRecoverTime);
        }
        break;
    }

    case IbisState::Recover:
        if (ibisTimer_ <= 0.0f) {
            setIbisState(IbisState::Hover, cfg::kF4IbisHoverTime);
        }
        break;

    case IbisState::Away:
        break;
    }
}

void BossScene::checkSlamContact() {
    if (isInvulnerable()) {
        return;
    }

    const float dx = playerPos_.x - ibisMarkPos_.x;
    const float dz = playerPos_.z - ibisMarkPos_.z;
    const float reach = cfg::kF4IbisSlamRadius + cfg::kF4PlayerRadius;

    if (dx * dx + dz * dz <= reach * reach) {
        takeHit();
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

void BossScene::checkParry() {
    // Only while actually dashing, and only once per charge - a second pass
    // through him should not re-arm a bonus that is already banked.
    if (dashTimer_ <= 0.0f || parryReady_) {
        return;
    }

    const float dx = playerPos_.x - rockyPos_.x;
    const float dz = playerPos_.z - rockyPos_.z;
    const float reach = cfg::kF4ParryRange + cfg::kF4PlayerRadius;

    if (dx * dx + dz * dz > reach * reach) {
        return;
    }

    parryReady_     = true;
    parryGlowTimer_ = cfg::kF4ParryGlowTime;
    parryTextTimer_ = cfg::kF4ParryTextTime;
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

    // A banked parry doubles this hit and is used up by it.
    const int damage = parryReady_ ? cfg::kF4ParryDamage : 1;
    parryReady_ = false;

    rockyHealth_ = std::max(0, rockyHealth_ - damage);

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

    parryReady_     = false;
    parryGlowTimer_ = 0.0f;
    parryTextTimer_ = 0.0f;

    ibisState_          = IbisState::Away;
    ibisTimer_          = 0.0f;
    ibisPos_            = glm::vec3(0.0f, cfg::kF4IbisHoverY, 0.0f);
    ibisFacingDeg_      = 0.0f;
    ibisMarkPos_        = glm::vec3(0.0f);
    ibisEntryTextTimer_ = 0.0f;

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

    parryGlowTimer_ = std::max(0.0f, parryGlowTimer_ - deltaTime);
    parryTextTimer_ = std::max(0.0f, parryTextTimer_ - deltaTime);

    updateDash(deltaTime);
    movePlayer(deltaTime);
    updateRocky(deltaTime);

    // After Rocky, so the health he just lost is what decides whether
    // Sebastian joins this frame. The two then run side by side, each on its
    // own timer - that is what makes their attacks overlap.
    updateIbis(deltaTime);
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
    drawSlamMark();   // on the deck, so it sits under everything standing on it
    drawRocky();
    drawIbis();
    drawPlayer();
    drawUi();
}

void BossScene::drawArena() {
    ctx().renderer.drawCylinderAt(
        glm::vec3(0.0f, -cfg::kF4DeckThickness * 0.5f, 0.0f),
        glm::vec3(cfg::kF4ArenaRadius * 2.0f, cfg::kF4DeckThickness, cfg::kF4ArenaRadius * 2.0f),
        cfg::kF4Deck);
}

void BossScene::drawRocky() {
    if (rockyState_ == RockyState::Beaten) {
        return;
    }

    float tint = 0.0f;
    glm::vec3 tintColor(1.0f);

    if (parryGlowTimer_ > 0.0f) {
        tint = 0.6f;
        tintColor = cfg::kF4RockyParryTint;
    } else if (rockyState_ == RockyState::Telegraph) {
        tint = 0.5f;
        tintColor = cfg::kF4RockyTelegraphTint;
    } else if (rockyState_ == RockyState::Stunned) {
        tint = 0.5f;
        tintColor = cfg::kF4RockyStunTint;
    }

    mascots::drawBull(ctx().renderer, rockyPos_, cfg::kF4RockyHeight,
                      rockyFacingDeg_, tint, tintColor);
}

void BossScene::drawSlamMark() {
    // Only while the warning is up. Once he is falling the bird itself is the
    // warning, and leaving the circle drawn would say "still safe to stand
    // here" at the exact moment it stops being true.
    if (ibisState_ != IbisState::Marking) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    // Brightens as the drop approaches, the same way Rocky's lane does, so
    // both telegraphs are read the same way.
    const float progress = (cfg::kF4IbisMarkTime > 0.0f)
        ? 1.0f - (ibisTimer_ / cfg::kF4IbisMarkTime)
        : 1.0f;

    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(ibisMarkPos_.x, 0.03f, ibisMarkPos_.z),
                            glm::vec3(cfg::kF4IbisSlamRadius * 2.0f, 0.02f,
                                      cfg::kF4IbisSlamRadius * 2.0f),
                            cfg::kF4SlamMarkColor,
                            cfg::kF4SlamMarkAlpha * (0.45f + progress * 0.55f),
                            1.0f);
    renderer.setDepthWrite(true);
}

void BossScene::drawIbis() {
    if (!ibisInFight()) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    // A shadow on the deck directly below him. While he is in the air this is
    // the only thing that says WHERE he is - the bird himself is drawn high up
    // and the camera makes that hard to judge.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(ibisPos_.x, 0.015f, ibisPos_.z),
                            glm::vec3(cfg::kF4IbisRadius * 2.2f, 0.02f,
                                      cfg::kF4IbisRadius * 2.2f),
                            cfg::kF4IbisShadow, cfg::kF4IbisShadowAlpha, 1.0f);
    renderer.setDepthWrite(true);

    mascots::drawIbis(renderer, ibisPos_, cfg::kF4IbisModelHeight, ibisFacingDeg_);
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
              px(24.0f), px(84.0f), hint, cfg::kTextPrimary);

    // Naming him matters: an unexplained second attacker reads as a bug.
    if (ibisInFight()) {
        text.draw("SEBASTIAN - CANNOT BE HURT, BEAT ROCKY",
                  px(24.0f), px(144.0f), hint, cfg::kIbisJersey);
    }

    // The parry says two things: that it happened, and that the bonus is still
    // in hand. Without the second line the player forgets they are holding it.
    if (parryTextTimer_ > 0.0f) {
        text.drawCentered("PARRY!", centerX, bottom * 0.30f,
                          px(cfg::kFloorNameScale), cfg::kF4RockyParryTint);
    }

    if (ibisEntryTextTimer_ > 0.0f) {
        text.drawCentered("SEBASTIAN JOINS THE FIGHT!", centerX, bottom * 0.24f,
                          px(cfg::kFloorNameScale), cfg::kIbisJersey);
    }

    if (parryReady_) {
        text.draw("PARRY READY - NEXT HIT DOES DOUBLE",
                  px(24.0f), px(114.0f), hint, cfg::kF4RockyParryTint);
    }

    if (outcomeTimer_ > 0.0f) {
        text.drawCentered(victory_ ? "ROCKY IS DOWN!" : "KNOCKED OUT - TRY AGAIN",
                          centerX, bottom * 0.42f, px(cfg::kFloorNameScale),
                          victory_ ? cfg::kTextComplete : cfg::kTextLocked);
    }

    text.drawCentered("WASD - MOVE     SPACE - DASH (THROUGH A CHARGE TO PARRY)",
                      centerX, bottom - px(90.0f), hint, cfg::kTextPrimary);
    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f), hint, cfg::kTextDim);

    text.end();
}
