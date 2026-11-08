// Runs the platformer: jumping with coyote time, input buffering and a jump cut,
// per-axis AABB collision against static and moving platforms, stompable and
// spiked enemies, item blocks, bounce pads, coins and lives with knockback.
// It builds the level from the tables in PlatformerLevel.h at init and draws the
// whole thing each frame.

#include "scenes/PlatformerScene.h"

#include "core/Camera.h"
#include "core/Config.h"
#include "core/GameProgress.h"
#include "core/Input.h"
#include "core/MascotModels.h"
#include "core/Renderer.h"
#include "core/SceneContext.h"
#include "core/SceneManager.h"
#include "scenes/PlatformerLevel.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {
float moveToward(float value, float target, float maxDelta) {
    if (std::abs(target - value) <= maxDelta) return target;
    return value + (target > value ? maxDelta : -maxDelta);
}

float accelerationFor(float current, float target, bool grounded) {
    using namespace cfg::plat;

    if (target == 0.0f) {
        return grounded ? kGroundDecel : kAirDecel;
    }
    const bool reversing = (current != 0.0f) && ((current > 0.0f) != (target > 0.0f));
    if (reversing) {
        return grounded ? kGroundTurn : kAirTurn;
    }
    return grounded ? kGroundAccel : kAirAccel;
}

}

bool PlatformerScene::init() {
    staticSolids_.clear();
    staticSolids_.reserve(static_cast<std::size_t>(level1::kPlatformCount));
    for (int i = 0; i < level1::kPlatformCount; ++i) {
        staticSolids_.push_back(
            Aabb::fromCenterSize(level1::kPlatforms[i].center, level1::kPlatforms[i].size));
    }

    movingPlatforms_.clear();
    for (int i = 0; i < level1::kMovingPlatformCount; ++i) {
        const level1::MovingPlatformDef& def = level1::kMovingPlatforms[i];

        MovingPlatform platform;
        platform.from = def.from;
        platform.to = def.to;
        platform.size = def.size;
        platform.speed = def.speed;
        platform.travel = def.startOffset;
        platform.center = glm::mix(def.from, def.to, def.startOffset);
        movingPlatforms_.push_back(platform);
    }

    coinTaken_.assign(static_cast<std::size_t>(level1::kCoinCount), false);
    itemBlockUsed_.assign(static_cast<std::size_t>(level1::kItemBlockCount), false);
    bouncePadSquash_.assign(static_cast<std::size_t>(level1::kBouncePadCount), 0.0f);
    poppedCoins_.clear();

    enemies_.clear();
    for (int i = 0; i < level1::kEnemyCount; ++i) {
        const level1::EnemyDef& def = level1::kEnemies[i];

        Enemy enemy;
        enemy.kind = def.kind;
        enemy.minX = def.minX;
        enemy.maxX = def.maxX;
        enemy.speed = def.speed;
        enemy.position = glm::vec3((def.minX + def.maxX) * 0.5f, def.feetY, def.z);
        enemy.direction = (i % 2 == 0) ? 1 : -1;
        enemies_.push_back(enemy);
    }

    coinsCollected_ = 0;
    enemiesDefeated_ = 0;
    lives_ = cfg::plat::kStartingLives;
    invulnerableTimer_ = 0.0f;
    falls_ = 0;
    reachedGoal_ = false;
    goalHoldTimer_ = 0.0f;
    elapsedTime_ = 0.0f;

    player_ = Aabb::fromCenterSize(level1::kSpawnPoint, cfg::plat::kPlayerSize);
    lastSafePosition_ = level1::kSpawnPoint;
    velocity_ = glm::vec3(0.0f);
    grounded_ = false;
    jumping_ = false;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    facingYaw_ = 0.0f;

    rebuildSolids();
    updateCamera(0.0f, true);
    return true;
}

void PlatformerScene::restartFromStart() {
    lives_ = cfg::plat::kStartingLives;
    lastSafePosition_ = level1::kSpawnPoint;
    invulnerableTimer_ = cfg::plat::kInvulnerableTime;
    respawn();
}

void PlatformerScene::respawn() {
    player_.center = lastSafePosition_;
    velocity_ = glm::vec3(0.0f);
    jumping_ = false;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    ++falls_;
}

void PlatformerScene::handleInput(const Input& input) {
    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        ctx().manager.requestScene(SceneId::LevelSelect);
        return;
    }

    if (input.wasPressed(GLFW_KEY_C)) {
        std::cout << "[Floor 1] DEBUG complete - " << coinsCollected_ << "/"
                  << level1::kTotalCoins << " coins, " << falls_ << " fall(s), "
                  << enemiesDefeated_ << " squashed\n";
        ctx().progress.markCompleted(0);
        ctx().manager.requestScene(SceneId::LevelSelect);
        return;
    }

    if (reachedGoal_) {
        moveInput_ = glm::vec2(0.0f);
        return;
    }

    if (input.wasPressed(GLFW_KEY_R)) {
        respawn();
    }

    moveInput_ = glm::vec2(0.0f);
    if (input.isDown(GLFW_KEY_A) || input.isDown(GLFW_KEY_LEFT))  moveInput_.x -= 1.0f;
    if (input.isDown(GLFW_KEY_D) || input.isDown(GLFW_KEY_RIGHT)) moveInput_.x += 1.0f;
    if (input.isDown(GLFW_KEY_W) || input.isDown(GLFW_KEY_UP))    moveInput_.y -= 1.0f;
    if (input.isDown(GLFW_KEY_S) || input.isDown(GLFW_KEY_DOWN))  moveInput_.y += 1.0f;

    if (moveInput_.x != 0.0f && moveInput_.y != 0.0f) {
        moveInput_ = glm::normalize(moveInput_);
    }

    jumpHeld_ = input.isDown(GLFW_KEY_SPACE);

    if (input.wasPressed(GLFW_KEY_SPACE)) {
        jumpBufferTimer_ = cfg::plat::kJumpBufferTime;
    }
}

void PlatformerScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    if (reachedGoal_) {
        goalHoldTimer_ -= deltaTime;
        if (goalHoldTimer_ <= 0.0f) {
            std::cout << "[Floor 1] REACHED THE GOAL - " << coinsCollected_ << "/"
                      << level1::kTotalCoins << " coins, " << falls_ << " fall(s), "
                      << enemiesDefeated_ << " squashed\n";
            ctx().progress.markCompleted(0);
            ctx().manager.requestScene(SceneId::LevelSelect);
        }
        updateCamera(deltaTime, false);
        return;
    }

    if (invulnerableTimer_ > 0.0f) {
        invulnerableTimer_ = std::max(0.0f, invulnerableTimer_ - deltaTime);
    }
    for (float& squash : bouncePadSquash_) {
        squash = std::max(0.0f, squash - deltaTime);
    }

    updateMovingPlatforms(deltaTime);
    rebuildSolids();
    updatePlayer(deltaTime);
    updateEnemies(deltaTime);
    resolveEnemyContacts();
    collectCoins();
    updatePoppedCoins(deltaTime);
    checkGoal();
    updateCamera(deltaTime, false);
}

void PlatformerScene::updateMovingPlatforms(float deltaTime) {
    for (MovingPlatform& platform : movingPlatforms_) {
        const Aabb before = Aabb::fromCenterSize(platform.center, platform.size);

        const bool riding =
            overlapsOnOtherAxes(player_, before, 1) &&
            std::abs(player_.bottom() - before.top()) < 0.14f;

        const float pathLength = glm::length(platform.to - platform.from);
        if (pathLength > 0.0001f) {
            platform.travel += static_cast<float>(platform.direction)
                             * (platform.speed / pathLength) * deltaTime;

            if (platform.travel >= 1.0f) {
                platform.travel = 1.0f;
                platform.direction = -1;
            } else if (platform.travel <= 0.0f) {
                platform.travel = 0.0f;
                platform.direction = 1;
            }
        }

        const glm::vec3 newCenter = glm::mix(platform.from, platform.to, platform.travel);
        platform.lastDelta = newCenter - platform.center;
        platform.center = newCenter;

        if (riding) {
            player_.center += platform.lastDelta;
        } else {
            const Aabb after = Aabb::fromCenterSize(platform.center, platform.size);
            if (overlaps(player_, after)) {
                for (int axis = 0; axis < 3; ++axis) {
                    if (std::abs(platform.lastDelta[axis]) < 0.0001f) continue;

                    if (platform.lastDelta[axis] > 0.0f) {
                        player_.center[axis] = after.max()[axis] + player_.halfExtents[axis];
                    } else {
                        player_.center[axis] = after.min()[axis] - player_.halfExtents[axis];
                    }
                }
            }
        }
    }
}

void PlatformerScene::rebuildSolids() {
    solids_ = staticSolids_;

    for (const MovingPlatform& platform : movingPlatforms_) {
        solids_.push_back(Aabb::fromCenterSize(platform.center, platform.size));
    }

    for (int i = 0; i < level1::kItemBlockCount; ++i) {
        solids_.push_back(Aabb::fromCenterSize(level1::kItemBlocks[i],
                                               cfg::plat::kItemBlockSize));
    }
    for (int i = 0; i < level1::kBouncePadCount; ++i) {
        solids_.push_back(Aabb::fromCenterSize(level1::kBouncePads[i].center,
                                               level1::kBouncePads[i].size));
    }
}

void PlatformerScene::updatePlayer(float deltaTime) {
    using namespace cfg::plat;

    const bool wasGrounded = grounded_;

    const glm::vec2 targetSpeed = moveInput_ * kMaxRunSpeed;

    velocity_.x = moveToward(velocity_.x, targetSpeed.x,
                             accelerationFor(velocity_.x, targetSpeed.x, grounded_) * deltaTime);
    velocity_.z = moveToward(velocity_.z, targetSpeed.y,
                             accelerationFor(velocity_.z, targetSpeed.y, grounded_) * deltaTime);

    const bool canJump = grounded_ || coyoteTimer_ > 0.0f;
    if (jumpBufferTimer_ > 0.0f && canJump) {
        velocity_.y = jumpSpeed();
        jumping_ = true;
        grounded_ = false;
        coyoteTimer_ = 0.0f;
        jumpBufferTimer_ = 0.0f;
    }

    if (jumping_ && !jumpHeld_ && velocity_.y > 0.0f) {
        velocity_.y *= kJumpCutMultiplier;
        jumping_ = false;
    }

    const float gravityScale = (velocity_.y < 0.0f) ? kFallGravityMultiplier : 1.0f;
    velocity_.y -= gravity() * gravityScale * deltaTime;

    velocity_.y = std::max(velocity_.y, -42.0f);

    const bool wasRising = velocity_.y > 0.0f;
    const bool hitVertical = moveAxis(player_, 1, velocity_.y * deltaTime, solids_);
    if (hitVertical) {
        grounded_ = (velocity_.y < 0.0f);
        if (grounded_) {
            jumping_ = false;
        }
        velocity_.y = 0.0f;

        if (wasRising) {
            punchItemBlockAbove();
        } else {
            checkBouncePads();
        }
    } else {
        grounded_ = false;
    }

    if (moveAxis(player_, 0, velocity_.x * deltaTime, solids_)) velocity_.x = 0.0f;
    if (moveAxis(player_, 2, velocity_.z * deltaTime, solids_)) velocity_.z = 0.0f;

    if (grounded_) {
        coyoteTimer_ = kCoyoteTime;
    } else if (coyoteTimer_ > 0.0f) {
        coyoteTimer_ = std::max(0.0f, coyoteTimer_ - deltaTime);
    }
    if (jumpBufferTimer_ > 0.0f) {
        jumpBufferTimer_ = std::max(0.0f, jumpBufferTimer_ - deltaTime);
    }

    const glm::vec2 horizontal(velocity_.x, velocity_.z);
    if (glm::length(horizontal) > 0.4f) {
        const float target = glm::degrees(std::atan2(velocity_.x, velocity_.z));

        float difference = target - facingYaw_;
        while (difference > 180.0f)  difference -= 360.0f;
        while (difference < -180.0f) difference += 360.0f;
        facingYaw_ += difference * std::min(1.0f, 12.0f * deltaTime);
    }

    if (grounded_ && !wasGrounded) {
        bool onStatic = false;
        for (const Aabb& solid : staticSolids_) {
            if (overlapsOnOtherAxes(player_, solid, 1) &&
                std::abs(player_.bottom() - solid.top()) < 0.14f) {
                onStatic = true;
                break;
            }
        }
        if (onStatic) {
            lastSafePosition_ = player_.center;
        }
    }

    if (player_.center.y < level1::kKillPlaneY) {
        respawn();
    }
}

Aabb PlatformerScene::enemyBox(const Enemy& enemy) const {
    const glm::vec3 size = cfg::plat::kEnemySize;
    return Aabb::fromCenterSize(enemy.position + glm::vec3(0.0f, size.y * 0.5f, 0.0f), size);
}

void PlatformerScene::updateEnemies(float deltaTime) {
    for (Enemy& enemy : enemies_) {
        if (!enemy.alive) {
            enemy.squashTimer = std::max(0.0f, enemy.squashTimer - deltaTime);
            continue;
        }

        enemy.position.x += static_cast<float>(enemy.direction) * enemy.speed * deltaTime;

        if (enemy.position.x <= enemy.minX) {
            enemy.position.x = enemy.minX;
            enemy.direction = 1;
        } else if (enemy.position.x >= enemy.maxX) {
            enemy.position.x = enemy.maxX;
            enemy.direction = -1;
        }
    }
}

void PlatformerScene::resolveEnemyContacts() {
    for (Enemy& enemy : enemies_) {
        if (!enemy.alive) continue;

        const Aabb box = enemyBox(enemy);
        if (!overlaps(player_, box)) continue;

        const bool comingDown = velocity_.y < 0.0f;
        const bool onTop = player_.bottom() >= box.top() - cfg::plat::kStompGrace;
        const bool stompable = (enemy.kind == level1::EnemyKind::Walker);

        if (comingDown && onTop && stompable) {
            enemy.alive = false;
            enemy.squashTimer = cfg::plat::kEnemySquashTime;
            ++enemiesDefeated_;

            velocity_.y = cfg::plat::kStompBounceSpeed;
            jumping_ = false;
            grounded_ = false;
        } else {
            takeDamage(enemy.position);
        }
    }
}

void PlatformerScene::takeDamage(const glm::vec3& source) {
    if (invulnerableTimer_ > 0.0f) {
        return;
    }

    --lives_;
    invulnerableTimer_ = cfg::plat::kInvulnerableTime;

    glm::vec3 away = player_.center - source;
    away.y = 0.0f;
    if (glm::length(away) < 0.001f) {
        away = glm::vec3(-1.0f, 0.0f, 0.0f);
    }
    away = glm::normalize(away);

    velocity_.x = away.x * cfg::plat::kHitKnockback;
    velocity_.z = away.z * cfg::plat::kHitKnockback;
    velocity_.y = cfg::plat::kHitKnockUp;
    grounded_ = false;
    jumping_ = false;

    if (lives_ <= 0) {
        restartFromStart();
    }
}

void PlatformerScene::punchItemBlockAbove() {
    for (int i = 0; i < level1::kItemBlockCount; ++i) {
        if (itemBlockUsed_[static_cast<std::size_t>(i)]) continue;

        const Aabb block = Aabb::fromCenterSize(level1::kItemBlocks[i],
                                                cfg::plat::kItemBlockSize);

        if (!overlapsOnOtherAxes(player_, block, 1)) continue;
        if (std::abs(player_.top() - block.min().y) > 0.25f) continue;

        itemBlockUsed_[static_cast<std::size_t>(i)] = true;
        ++coinsCollected_;

        PoppedCoin coin;
        coin.origin = glm::vec3(block.center.x, block.max().y, block.center.z);
        coin.timer = cfg::plat::kPoppedCoinTime;
        poppedCoins_.push_back(coin);
        return;
    }
}

void PlatformerScene::checkBouncePads() {
    for (int i = 0; i < level1::kBouncePadCount; ++i) {
        const Aabb pad = Aabb::fromCenterSize(level1::kBouncePads[i].center,
                                              level1::kBouncePads[i].size);

        if (!overlapsOnOtherAxes(player_, pad, 1)) continue;
        if (std::abs(player_.bottom() - pad.top()) > 0.25f) continue;

        velocity_.y = cfg::plat::kBouncePadSpeed;
        grounded_ = false;
        jumping_ = false;
        bouncePadSquash_[static_cast<std::size_t>(i)] = cfg::plat::kBouncePadSquash;
        return;
    }
}

void PlatformerScene::updatePoppedCoins(float deltaTime) {
    for (PoppedCoin& coin : poppedCoins_) {
        coin.timer -= deltaTime;
    }
    poppedCoins_.erase(
        std::remove_if(poppedCoins_.begin(), poppedCoins_.end(),
                       [](const PoppedCoin& coin) { return coin.timer <= 0.0f; }),
        poppedCoins_.end());
}

void PlatformerScene::collectCoins() {
    for (int i = 0; i < level1::kCoinCount; ++i) {
        if (coinTaken_[static_cast<std::size_t>(i)]) continue;

        if (glm::distance(player_.center, level1::kCoins[i]) < cfg::plat::kCoinRadius + 0.55f) {
            coinTaken_[static_cast<std::size_t>(i)] = true;
            ++coinsCollected_;
        }
    }
}

void PlatformerScene::checkGoal() {
    const Aabb goal = Aabb::fromCenterSize(level1::kGoalPoint, level1::kGoalSize);
    if (!reachedGoal_ && overlaps(player_, goal)) {
        reachedGoal_ = true;
        goalHoldTimer_ = cfg::plat::kGoalHoldTime;
    }
}

void PlatformerScene::updateCamera(float deltaTime, bool snap) {
    const glm::vec3 target = player_.center + cfg::plat::kCameraOffset;

    if (snap) {
        cameraPosition_ = target;
        return;
    }

    const float blend = 1.0f - std::exp(-cfg::plat::kCameraFollow * deltaTime);
    cameraPosition_ += (target - cameraPosition_) * blend;
}

bool PlatformerScene::findGroundShadow(float& outY) const {
    bool found = false;
    float best = 0.0f;

    for (const Aabb& solid : solids_) {
        if (!overlapsOnOtherAxes(player_, solid, 1)) continue;
        if (solid.top() > player_.bottom() + 0.05f) continue;

        if (!found || solid.top() > best) {
            best = solid.top();
            found = true;
        }
    }

    outY = best;
    return found;
}

void PlatformerScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    renderer.drawBackground(glm::mix(cfg::kBgTop, cfg::kF1Wall, 0.55f),
                            glm::mix(cfg::kBgBottom, cfg::kF1Wall, 0.25f),
                            cfg::kBgGlow, glm::vec2(0.0f, 0.1f),
                            cfg::kBgGlowRadius, 0.30f);

    Camera camera(cameraPosition_, player_.center + cfg::plat::kCameraLookAt);
    camera.setFovDegrees(cfg::plat::kCameraFovDeg);
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawWorld();
    drawHud();
}

void PlatformerScene::drawWorld() {
    Renderer& renderer = ctx().renderer;

    for (int i = 0; i < level1::kPlatformCount; ++i) {
        const level1::PlatformDef& platform = level1::kPlatforms[i];
        const glm::vec3 top = platform.warm ? cfg::plat::kPlatformWarm
                                            : cfg::plat::kPlatformCool;

        renderer.drawBoxAt(platform.center, platform.size, cfg::plat::kPlatformEdge);
        renderer.drawBoxAt(platform.center + glm::vec3(0.0f, platform.size.y * 0.5f, 0.0f),
                           glm::vec3(platform.size.x, 0.16f, platform.size.z), top);
    }

    for (const MovingPlatform& platform : movingPlatforms_) {
        renderer.drawBoxAt(platform.center, platform.size, cfg::plat::kPlatformEdge);
        renderer.drawBoxAt(platform.center + glm::vec3(0.0f, platform.size.y * 0.5f, 0.0f),
                           glm::vec3(platform.size.x, 0.16f, platform.size.z),
                           cfg::plat::kMovingPlatform);
    }

    for (int i = 0; i < level1::kItemBlockCount; ++i) {
        const bool used = itemBlockUsed_[static_cast<std::size_t>(i)];
        const glm::vec3 center = level1::kItemBlocks[i];
        const glm::vec3 size = cfg::plat::kItemBlockSize;

        if (used) {
            renderer.drawBoxAt(center, size, cfg::plat::kItemBlockSpent);
            continue;
        }

        const float pulse = 0.5f + 0.5f * std::sin(elapsedTime_ * 4.0f
                                                   + static_cast<float>(i));
        renderer.drawBoxAt(center, size, cfg::plat::kItemBlockLive,
                           1.0f, 0.20f + 0.25f * pulse);

        renderer.drawBoxAt(center, glm::vec3(size.x * 1.04f, size.y * 0.22f, size.z * 1.04f),
                           cfg::plat::kItemBlockTrim, 1.0f, 0.35f);
    }

    for (int i = 0; i < level1::kBouncePadCount; ++i) {
        const level1::BouncePadDef& pad = level1::kBouncePads[i];
        const float squash = bouncePadSquash_[static_cast<std::size_t>(i)]
                           / cfg::plat::kBouncePadSquash;

        const float height = pad.size.y * (1.0f - 0.55f * squash);

        renderer.drawBoxAt(pad.center, glm::vec3(pad.size.x, pad.size.y * 0.4f, pad.size.z),
                           cfg::plat::kPlatformEdge);
        renderer.drawCylinderAt(glm::vec3(pad.center.x,
                                          pad.center.y - pad.size.y * 0.5f + height * 0.5f,
                                          pad.center.z),
                                glm::vec3(pad.size.x * 0.9f, height, pad.size.z * 0.9f),
                                cfg::plat::kBouncePadColor, 1.0f, 0.45f);
    }

    for (const PoppedCoin& coin : poppedCoins_) {
        const float t = 1.0f - coin.timer / cfg::plat::kPoppedCoinTime;
        const float rise = cfg::plat::kPoppedCoinRise * std::sin(t * 1.5707963f);

        renderer.setDepthWrite(false);
        renderer.drawCylinder(
            makeTransform(coin.origin + glm::vec3(0.0f, 0.4f + rise, 0.0f),
                          glm::vec3(0.46f, 0.09f, 0.46f),
                          glm::vec3(90.0f, elapsedTime_ * 420.0f, 0.0f)),
            cfg::kF1Coin, 1.0f - t, 1.0f);
        renderer.setDepthWrite(true);
    }

    for (int i = 0; i < level1::kCoinCount; ++i) {
        if (coinTaken_[static_cast<std::size_t>(i)]) continue;

        const float bob = std::sin(elapsedTime_ * 2.4f + static_cast<float>(i)) * 0.12f;
        renderer.drawCylinder(
            makeTransform(level1::kCoins[i] + glm::vec3(0.0f, bob, 0.0f),
                          glm::vec3(0.52f, 0.10f, 0.52f),
                          glm::vec3(90.0f, elapsedTime_ * cfg::plat::kCoinSpinRate, 0.0f)),
            cfg::kF1Coin, 1.0f, 0.45f);
    }

    const float goalPulse = 0.5f + 0.5f * std::sin(elapsedTime_ * 3.0f);
    renderer.drawCylinderAt(level1::kGoalPoint,
                            glm::vec3(level1::kGoalSize.x * 1.6f, level1::kGoalSize.y,
                                      level1::kGoalSize.z * 1.6f),
                            cfg::plat::kGoalColor, 1.0f, 0.35f + 0.35f * goalPulse);
    for (int i = 0; i < 3; ++i) {
        const float y = 0.7f + static_cast<float>(i) * 0.5f;
        renderer.drawCylinderAt(level1::kGoalPoint + glm::vec3(0.0f, y, 0.0f),
                                glm::vec3(1.1f - static_cast<float>(i) * 0.22f, 0.07f,
                                          1.1f - static_cast<float>(i) * 0.22f),
                                cfg::plat::kGoalColor,
                                0.30f + 0.20f * goalPulse, 1.0f);
    }

    float shadowY = 0.0f;
    if (findGroundShadow(shadowY)) {
        const float drop = std::clamp(player_.bottom() - shadowY, 0.0f, 7.0f);
        const float closeness = 1.0f - drop / 7.0f;

        renderer.setDepthWrite(false);
        renderer.drawCylinderAt(glm::vec3(player_.center.x, shadowY + 0.03f, player_.center.z),
                                glm::vec3(0.95f * (0.45f + 0.55f * closeness), 0.02f,
                                          0.95f * (0.45f + 0.55f * closeness)),
                                cfg::plat::kShadowColor, 0.30f + 0.32f * closeness, 1.0f);
        renderer.setDepthWrite(true);
    }

    drawEnemies();

    const bool blinkedOut = invulnerableTimer_ > 0.0f &&
                            std::fmod(invulnerableTimer_ * 11.0f, 1.0f) < 0.45f;
    if (!blinkedOut) {
        mascots::drawRoary(renderer,
                           glm::vec3(player_.center.x, player_.bottom(), player_.center.z),
                           cfg::plat::kPlayerSize.y, facingYaw_);
    }
}

void PlatformerScene::drawEnemies() {
    Renderer& renderer = ctx().renderer;
    const glm::vec3 size = cfg::plat::kEnemySize;

    for (const Enemy& enemy : enemies_) {
        if (!enemy.alive && enemy.squashTimer <= 0.0f) continue;

        const bool spiker = (enemy.kind == level1::EnemyKind::Spiker);
        const glm::vec3 body = spiker ? cfg::plat::kSpikerBody : cfg::plat::kWalkerBody;

        float squash = 1.0f;
        if (!enemy.alive) {
            squash = enemy.squashTimer / cfg::plat::kEnemySquashTime;
        }

        const float height = size.y * (0.18f + 0.82f * squash);
        const float width = size.x * (1.0f + 0.5f * (1.0f - squash));
        const glm::vec3 center = enemy.position + glm::vec3(0.0f, height * 0.5f, 0.0f);

        const float waddle = enemy.alive
            ? std::sin(elapsedTime_ * 9.0f + enemy.position.x) * 7.0f : 0.0f;

        renderer.drawSphere(makeTransform(center, glm::vec3(width, height, size.z),
                                          glm::vec3(0.0f, 0.0f, waddle)),
                            body);

        if (!enemy.alive) continue;

        for (int side = 0; side < 2; ++side) {
            const float x = (side == 0 ? -0.24f : 0.24f);
            renderer.drawSphereAt(enemy.position + glm::vec3(x, 0.10f, 0.0f),
                                  glm::vec3(0.26f, 0.18f, 0.30f),
                                  spiker ? cfg::plat::kSpikerSpike : cfg::plat::kWalkerFoot);
        }

        const float lookZ = size.z * 0.42f;
        mascots::drawEyePair(renderer,
                             enemy.position + glm::vec3(0.0f, height * 0.62f, lookZ),
                             0.30f, 0.15f, cfg::kEyeWhite, cfg::kEyeDark);

        if (spiker) {
            for (int i = 0; i < 4; ++i) {
                const float angle = 6.2831853f * static_cast<float>(i) / 4.0f;
                renderer.drawPyramidAt(
                    enemy.position + glm::vec3(std::sin(angle) * 0.20f,
                                               height + 0.10f,
                                               std::cos(angle) * 0.20f),
                    glm::vec3(0.20f, 0.30f, 0.20f), cfg::plat::kSpikerSpike);
            }
        }
    }
}

void PlatformerScene::drawHud() {
    Renderer& renderer = ctx().renderer;
    TextRenderer& text = renderer.text();

    const float width = static_cast<float>(ctx().framebufferWidth);
    const float height = static_cast<float>(ctx().framebufferHeight);
    const float centerX = width * 0.5f;
    const auto px = [this](float designPixels) { return designPixels * uiScale_; };

    text.begin(ctx().framebufferWidth, ctx().framebufferHeight);

    const cfg::FloorInfo& info = cfg::kFloorInfo[0];
    text.drawCentered(info.name, centerX, px(26.0f),
                      px(cfg::kFloorNameScale), cfg::kTextPrimary);
    text.drawCentered(info.description, centerX, px(72.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    const std::string coins = "COINS " + std::to_string(coinsCollected_) +
                              "/" + std::to_string(level1::kTotalCoins);
    text.draw(coins, width - px(24.0f) - TextRenderer::measureWidth(coins, px(3.0f)),
              px(26.0f), px(3.0f), cfg::kFiuGold);

    std::string livesLabel = "LIVES ";
    for (int i = 0; i < cfg::plat::kStartingLives; ++i) {
        livesLabel += (i < lives_) ? "*" : "-";
    }
    text.draw(livesLabel,
              width - px(24.0f) - TextRenderer::measureWidth(livesLabel, px(3.0f)),
              px(62.0f), px(3.0f),
              lives_ > 1 ? cfg::kTextComplete : cfg::kTextLocked);

    const std::string fallsLabel = "FALLS " + std::to_string(falls_) +
                                   "   SQUASHED " + std::to_string(enemiesDefeated_);
    text.draw(fallsLabel,
              width - px(24.0f) - TextRenderer::measureWidth(fallsLabel, px(cfg::kHintScale)),
              px(100.0f), px(cfg::kHintScale), cfg::kTextDim);

    if (reachedGoal_) {
        const std::string banner = (coinsCollected_ == level1::kTotalCoins)
            ? "FLOOR CLEARED - ALL COINS!"
            : "FLOOR CLEARED!";
        text.drawCentered(banner, centerX, height * 0.42f,
                          px(5.0f), cfg::kTextComplete);
    }

    text.drawCentered("WASD - MOVE     SPACE - JUMP     R - RESPAWN",
                      centerX, height - px(78.0f), px(cfg::kHintScale), cfg::kTextPrimary);
    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, height - px(48.0f), px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
