// StealthScene.cpp - Floor 3, "Silent Paws"
// Each frame: handleInput() reads keys, update() moves everything, render() draws.
// The room is flat on the XZ plane (y = 0); a facing of 0 degrees looks down +Z.

#include "scenes/StealthScene.h"

#include "core/Angles.h"
#include "core/MascotModels.h"

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

const float kWallStep = 0.1f;

float floorDistance(const glm::vec3& a, const glm::vec3& b) {
    const float dx = b.x - a.x;
    const float dz = b.z - a.z;
    return std::sqrt(dx * dx + dz * dz);
}

float angleToward(const glm::vec3& from, const glm::vec3& to) {
    return glm::degrees(std::atan2(to.x - from.x, to.z - from.z));
}

glm::mat4 visionConeTransform(const glm::vec3& apex, float facingDeg,
                              float range, float width, float thickness) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, apex);
    model = glm::rotate(model, glm::radians(facingDeg), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    model = glm::translate(model, glm::vec3(0.0f, -range * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(width, range, thickness));
    return model;
}

} // namespace

bool StealthScene::init() {
    playerPos_   = cfg::kF3StartPos;
    moveDir_     = glm::vec2(0.0f);
    facingDeg_   = 180.0f;
    sneaking_    = false;

    resetGuard();
    playerSeen_ = false;

    suspicion_        = 0.0f;
    sightLostTimer_   = 0.0f;
    caughtFlashTimer_ = 0.0f;

    itemCollected_.fill(false);
    lockedFlashTimer_ = 0.0f;

    rocksLeft_    = cfg::kF3RockCount;
    aiming_       = false;
    aimValid_     = false;
    aimBlocked_   = false;
    rockInFlight_ = false;
    noiseTimer_   = 0.0f;

    return true;
}

void StealthScene::returnToMenu() {
    ctx().manager.requestScene(SceneId::LevelSelect);
}

void StealthScene::handleInput(const Input& input) {
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

    sneaking_ = input.isDown(GLFW_KEY_LEFT_SHIFT)
             || input.isDown(GLFW_KEY_RIGHT_SHIFT);

    if (input.wasPressed(GLFW_KEY_E)) {
        if (aiming_) {
            aiming_ = false;
        } else if (rocksLeft_ > 0 && !rockInFlight_) {
            aiming_ = true;
        }
    }

    if (aiming_) {
        updateAim(input);

        if (input.wasMouseClicked()) {
            throwRock();
        }
    }
}

bool StealthScene::pointInsideWall(const glm::vec3& point) {
    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];

        const bool insideX = std::fabs(point.x - wall.centerX) < wall.sizeX * 0.5f;
        const bool insideZ = std::fabs(point.z - wall.centerZ) < wall.sizeZ * 0.5f;

        if (insideX && insideZ) {
            return true;
        }
    }
    return false;
}

// Steps along the line and stops at the first point inside a wall
glm::vec3 StealthScene::walkUntilWall(const glm::vec3& from, const glm::vec3& to,
                                      bool& outBlocked) {
    const float distance = floorDistance(from, to);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / kWallStep)));

    glm::vec3 lastClear = from;

    for (int i = 1; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const glm::vec3 point = from + (to - from) * t;

        if (pointInsideWall(point)) {
            outBlocked = true;
            return lastClear;
        }
        lastClear = point;
    }

    outBlocked = false;
    return to;
}

glm::vec3 StealthScene::clampToRoom(const glm::vec3& position, float radius) const {
    const float limitX = cfg::kF3RoomHalfX - radius;
    const float limitZ = cfg::kF3RoomHalfZ - radius;

    return glm::vec3(std::clamp(position.x, -limitX, limitX),
                     position.y,
                     std::clamp(position.z, -limitZ, limitZ));
}

void StealthScene::resolveInnerWalls(glm::vec3& position, int axis, float delta,
                                     float radius) const {
    if (delta == 0.0f) {
        return;
    }

    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];

        const float minX = wall.centerX - wall.sizeX * 0.5f;
        const float maxX = wall.centerX + wall.sizeX * 0.5f;
        const float minZ = wall.centerZ - wall.sizeZ * 0.5f;
        const float maxZ = wall.centerZ + wall.sizeZ * 0.5f;

        const bool overlapping =
            position.x + radius > minX && position.x - radius < maxX &&
            position.z + radius > minZ && position.z - radius < maxZ;

        if (!overlapping) {
            continue;
        }

        if (axis == 0) {
            position.x = (delta > 0.0f) ? minX - radius : maxX + radius;
        } else {
            position.z = (delta > 0.0f) ? minZ - radius : maxZ + radius;
        }
    }
}

void StealthScene::movePlayer(float deltaTime) {
    if (moveDir_ == glm::vec2(0.0f)) {
        return;
    }

    const float speed  = sneaking_ ? cfg::kF3SneakSpeed : cfg::kF3WalkSpeed;
    const float radius = cfg::kF3PlayerRadius;

    const float deltaX = moveDir_.x * speed * deltaTime;
    const float deltaZ = moveDir_.y * speed * deltaTime;

    // One axis at a time, so Roary slides along walls instead of sticking
    playerPos_.x += deltaX;
    resolveInnerWalls(playerPos_, 0, deltaX, radius);

    playerPos_.z += deltaZ;
    resolveInnerWalls(playerPos_, 2, deltaZ, radius);

    playerPos_ = clampToRoom(playerPos_, radius);

    facingDeg_ = glm::degrees(std::atan2(moveDir_.x, moveDir_.y));
}

void StealthScene::collectNearbyItems() {
    for (int i = 0; i < cfg::kF3ItemCount; ++i) {
        if (itemCollected_[i]) {
            continue;
        }

        if (floorDistance(playerPos_, cfg::kF3ItemPositions[i])
                <= cfg::kF3ItemPickupRadius) {
            itemCollected_[i] = true;
        }
    }
}

int StealthScene::collectedCount() const {
    return static_cast<int>(
        std::count(itemCollected_.begin(), itemCollected_.end(), true));
}

bool StealthScene::doorIsOpen() const {
    return collectedCount() == cfg::kF3ItemCount;
}

void StealthScene::checkDoor() {
    const glm::vec3 door(cfg::kF3RoomHalfX, 0.0f, cfg::kF3DoorZ);

    if (floorDistance(playerPos_, door) > cfg::kF3DoorTriggerRadius) {
        return;
    }

    if (doorIsOpen()) {
        ctx().progress.markCompleted(kFloorIndex);
        returnToMenu();
        return;
    }

    lockedFlashTimer_ = cfg::kF3LockedFlashTime;
}

void StealthScene::resetGuard() {
    guardPos_        = cfg::kF3GuardStart;
    guardFacingDeg_  = cfg::kF3GuardFacing;
    patrolIndex_     = 0;
    guardPauseTimer_ = 0.0f;
    guardAlertTimer_ = 0.0f;
}

// Distracted: look at the noise. Pausing: wait. Otherwise: walk to the next point.
void StealthScene::updateGuard(float deltaTime) {
    float targetFacingDeg = guardFacingDeg_;

    if (guardIsDistracted()) {
        guardAlertTimer_ = std::max(0.0f, guardAlertTimer_ - deltaTime);

        if (floorDistance(guardPos_, guardAlertTarget_) > 0.01f) {
            targetFacingDeg = angleToward(guardPos_, guardAlertTarget_);
        }
    } else if (guardPauseTimer_ > 0.0f) {
        guardPauseTimer_ = std::max(0.0f, guardPauseTimer_ - deltaTime);
    } else {
        const cfg::F3PatrolPoint& point = cfg::kF3PatrolPoints[patrolIndex_];
        const float distance = floorDistance(guardPos_, point.position);
        const float step     = cfg::kF3GuardSpeed * deltaTime;

        if (distance <= step) {
            guardPos_        = point.position;
            guardPauseTimer_ = point.pauseSeconds;
            patrolIndex_     = (patrolIndex_ + 1) % cfg::kF3PatrolCount;
        } else {
            const glm::vec3 direction = (point.position - guardPos_) / distance;
            guardPos_ += direction * step;
            targetFacingDeg = angleToward(guardPos_, point.position);
        }
    }

    guardFacingDeg_ = targetFacingDeg;
}

// Seen = in range, inside the cone's angle, and no wall in between
bool StealthScene::guardCanSeePlayer() const {
    const float distance = floorDistance(guardPos_, playerPos_);

    if (distance > cfg::kF3VisionRange) {
        return false;
    }
    if (distance < 0.01f) {
        return true;
    }

    const float angleToPlayer = angleToward(guardPos_, playerPos_);
    const float offCentre = std::fabs(shortestAngleDelta(guardFacingDeg_, angleToPlayer));

    if (offCentre > cfg::kF3VisionHalfAngleDeg) {
        return false;
    }

    bool blocked = false;
    walkUntilWall(guardPos_, playerPos_, blocked);
    return !blocked;
}

void StealthScene::updateSuspicion(float deltaTime) {
    caughtFlashTimer_ = std::max(0.0f, caughtFlashTimer_ - deltaTime);

    if (playerSeen_) {
        const float distance  = floorDistance(guardPos_, playerPos_);
        // Fills faster the closer Roary is (lerp between the far and near rates)
        const float closeness = 1.0f - std::clamp(distance / cfg::kF3VisionRange, 0.0f, 1.0f);

        float rate = cfg::kF3SuspicionFarRate
                   + (cfg::kF3SuspicionNearRate - cfg::kF3SuspicionFarRate) * closeness;

        if (sneaking_) {
            rate *= cfg::kF3SuspicionSneakMul;
        }

        suspicion_ += rate * deltaTime;
        sightLostTimer_ = cfg::kF3SuspicionGrace;

        if (suspicion_ >= 1.0f) {
            getCaught();
        }
    } else if (sightLostTimer_ > 0.0f) {
        sightLostTimer_ = std::max(0.0f, sightLostTimer_ - deltaTime);
    } else {
        suspicion_ = std::max(0.0f, suspicion_ - cfg::kF3SuspicionDrain * deltaTime);
    }
}

void StealthScene::getCaught() {
    playerPos_ = cfg::kF3StartPos;
    moveDir_   = glm::vec2(0.0f);
    facingDeg_ = 180.0f;

    suspicion_        = 0.0f;
    sightLostTimer_   = 0.0f;
    playerSeen_       = false;
    caughtFlashTimer_ = cfg::kF3CaughtFlashTime;

    aiming_       = false;
    rockInFlight_ = false;
    noiseTimer_   = 0.0f;

    resetGuard();
}

Camera StealthScene::makeCamera() const {
    Camera camera(cfg::kF3CameraPos, cfg::kF3CameraTarget);
    camera.setFovDegrees(cfg::kF3CameraFovDeg);
    return camera;
}

// The mouse pixel is a ray from the camera; find where it hits the floor (y = 0)
bool StealthScene::screenToFloor(const glm::vec2& screenPos,
                                 glm::vec3& outFloor) const {
    const float width  = static_cast<float>(ctx().framebufferWidth);
    const float height = static_cast<float>(ctx().framebufferHeight);

    const Camera camera = makeCamera();
    const glm::mat4 view       = camera.view();
    const glm::mat4 projection = camera.perspective(ctx().framebufferWidth,
                                                    ctx().framebufferHeight);
    const glm::vec4 viewport(0.0f, 0.0f, width, height);

    const float x = screenPos.x;
    const float y = height - screenPos.y;

    const glm::vec3 nearPoint = glm::unProject(glm::vec3(x, y, 0.0f), view, projection, viewport);
    const glm::vec3 farPoint  = glm::unProject(glm::vec3(x, y, 1.0f), view, projection, viewport);

    const float rise = farPoint.y - nearPoint.y;
    if (std::fabs(rise) < 1e-6f) {
        return false;
    }

    const float t = -nearPoint.y / rise;
    if (t < 0.0f) {
        return false;
    }

    outFloor = nearPoint + (farPoint - nearPoint) * t;
    outFloor.y = 0.0f;
    return true;
}

void StealthScene::updateAim(const Input& input) {
    glm::vec3 target(0.0f);
    aimValid_ = screenToFloor(input.mousePosition(), target);

    if (!aimValid_) {
        return;
    }

    target = clampToRoom(target, cfg::kF3RockRadius * 2.0f);

    const float distance = floorDistance(playerPos_, target);
    if (distance > cfg::kF3RockThrowRange) {
        target = playerPos_ + (target - playerPos_) * (cfg::kF3RockThrowRange / distance);
    }

    aimImpact_ = walkUntilWall(playerPos_, target, aimBlocked_);
    aimImpact_.y = 0.0f;
}

void StealthScene::throwRock() {
    if (rocksLeft_ <= 0 || rockInFlight_ || !aimValid_) {
        return;
    }

    --rocksLeft_;
    aiming_ = false;

    rockFrom_     = playerPos_;
    rockTo_       = aimImpact_;
    rockElapsed_  = 0.0f;
    rockInFlight_ = true;

    rockDuration_ = std::max(0.18f, floorDistance(rockFrom_, rockTo_) / cfg::kF3RockSpeed);

    facingDeg_ = angleToward(rockFrom_, rockTo_);
}

void StealthScene::updateRock(float deltaTime) {
    noiseTimer_ = std::max(0.0f, noiseTimer_ - deltaTime);

    if (!rockInFlight_) {
        return;
    }

    rockElapsed_ += deltaTime;

    if (rockElapsed_ >= rockDuration_) {
        rockInFlight_ = false;
        raiseNoise(rockTo_);
    }
}

void StealthScene::raiseNoise(const glm::vec3& position) {
    noisePos_   = position;
    noiseTimer_ = cfg::kF3NoiseDuration;

    // Hearing ignores walls, so a rock through a doorway still distracts him
    if (floorDistance(guardPos_, position) <= cfg::kF3NoiseRadius) {
        guardAlertTimer_  = cfg::kF3GuardAlertTime;
        guardAlertTarget_ = position;
    }
}

void StealthScene::update(float deltaTime) {
    // Order matters: each step uses the results of the ones above it
    movePlayer(deltaTime);
    collectNearbyItems();
    checkDoor();
    updateRock(deltaTime);
    updateGuard(deltaTime);
    playerSeen_ = guardCanSeePlayer();
    updateSuspicion(deltaTime);

    lockedFlashTimer_ = std::max(0.0f, lockedFlashTimer_ - deltaTime);
}

void StealthScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width  = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    renderer.drawBackground(
        glm::mix(cfg::kBgTop, cfg::kF3Wall, 0.70f),
        glm::mix(cfg::kBgBottom, cfg::kF3Wall, 0.30f),
        cfg::kF3Light,
        glm::vec2(0.0f, 0.1f),
        cfg::kBgGlowRadius,
        0.10f);

    const Camera camera = makeCamera();
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawRoom();
    drawDoor();
    drawItems();
    drawGuard();
    drawAlertMark();
    drawRoary();
    drawRock();

    drawVisionCone();
    drawNoise();
    drawAimMarker();

    drawUi();
}

void StealthScene::drawRoom() {
    Renderer& renderer = ctx().renderer;

    const float halfX  = cfg::kF3RoomHalfX;
    const float halfZ  = cfg::kF3RoomHalfZ;
    const float thick  = cfg::kF3WallThick;
    const float height = cfg::kF3WallHeight;

    renderer.drawBoxAt(glm::vec3(0.0f, -0.10f, 0.0f),
                       glm::vec3(halfX * 2.0f, 0.20f, halfZ * 2.0f),
                       cfg::kF3Floor);

    const auto drawWall = [&](float centerX, float centerZ,
                              float sizeX, float sizeZ, float wallHeight) {
        renderer.drawBoxAt(glm::vec3(centerX, wallHeight * 0.5f, centerZ),
                           glm::vec3(sizeX, wallHeight, sizeZ),
                           cfg::kF3WallIn);
    };

    const float spanX = halfX * 2.0f + thick * 2.0f;
    const float spanZ = halfZ * 2.0f;

    drawWall(0.0f, -halfZ - thick * 0.5f, spanX, thick, height);
    drawWall(-halfX - thick * 0.5f, 0.0f, thick, spanZ, height);
    drawWall( halfX + thick * 0.5f, 0.0f, thick, spanZ, height);

    drawWall(0.0f, halfZ + thick * 0.5f, spanX, thick, cfg::kF3NearWallHeight);

    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];
        drawWall(wall.centerX, wall.centerZ, wall.sizeX, wall.sizeZ,
                 cfg::kF3InnerWallHeight);
    }
}

void StealthScene::drawDoor() {
    const glm::vec3 color = doorIsOpen() ? cfg::kF3DoorOpen : cfg::kF3DoorLocked;

    ctx().renderer.drawBoxAt(
        glm::vec3(cfg::kF3RoomHalfX + cfg::kF3WallThick * 0.5f,
                  cfg::kF3DoorHeight * 0.5f,
                  cfg::kF3DoorZ),
        glm::vec3(cfg::kF3WallThick * 1.2f, cfg::kF3DoorHeight, cfg::kF3DoorWidth),
        color, 1.0f, 0.5f);
}

void StealthScene::drawItems() {
    for (int i = 0; i < cfg::kF3ItemCount; ++i) {
        if (itemCollected_[i]) {
            continue;
        }

        const glm::vec3& item = cfg::kF3ItemPositions[i];
        ctx().renderer.drawSphereAt(glm::vec3(item.x, cfg::kF3ItemHoverY, item.z),
                                    glm::vec3(cfg::kF3ItemSize),
                                    cfg::kF3ItemPaw);
    }
}

void StealthScene::drawGuard() {
    Renderer& renderer = ctx().renderer;

    const float bodyHeight = cfg::kF3GuardBodyHeight;
    const float headSize   = cfg::kF3GuardHeadSize;
    const float width      = cfg::kF3GuardRadius * 2.0f;

    renderer.drawBoxAt(guardPos_ + glm::vec3(0.0f, bodyHeight * 0.5f, 0.0f),
                       glm::vec3(width, bodyHeight, width),
                       cfg::kF3GuardBody);

    renderer.drawBoxAt(guardPos_ + glm::vec3(0.0f, bodyHeight + headSize * 0.5f, 0.0f),
                       glm::vec3(headSize),
                       cfg::kF3GuardHead);
}

void StealthScene::drawAlertMark() {
    if (!guardIsDistracted()) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    const float baseY = cfg::kF3GuardBodyHeight + cfg::kF3GuardHeadSize + cfg::kF3AlertMarkY;

    renderer.drawBoxAt(glm::vec3(guardPos_.x, baseY + 0.68f, guardPos_.z),
                       glm::vec3(0.20f, 0.70f, 0.20f),
                       cfg::kF3AlertMark, 1.0f, 0.85f);

    renderer.drawBoxAt(glm::vec3(guardPos_.x, baseY, guardPos_.z),
                       glm::vec3(0.22f),
                       cfg::kF3AlertMark, 1.0f, 0.85f);
}

void StealthScene::drawRoary() {
    mascots::drawRoary(ctx().renderer, playerPos_,
                       mascots::kRoaryNaturalHeight * cfg::kF3PlayerScale,
                       facingDeg_);
}

void StealthScene::drawRock() {
    if (!rockInFlight_) {
        return;
    }

    const float t = std::min(1.0f, rockElapsed_ / rockDuration_);
    const glm::vec3 position = rockFrom_ + (rockTo_ - rockFrom_) * t;

    ctx().renderer.drawSphereAt(glm::vec3(position.x, 0.5f, position.z),
                                glm::vec3(cfg::kF3RockRadius * 2.0f),
                                cfg::kF3RockColor);
}

void StealthScene::drawVisionCone() {
    Renderer& renderer = ctx().renderer;

    const float range = cfg::kF3VisionRange;

    const float width = 2.0f * range * std::tan(glm::radians(cfg::kF3VisionHalfAngleDeg));

    glm::vec3 coneColor = cfg::kF3ConeCalm;
    if (playerSeen_) {
        coneColor = cfg::kF3ConeAlert;
    } else if (guardIsDistracted()) {
        coneColor = cfg::kF3ConeAlerted;
    }

    const glm::vec3 apex(guardPos_.x, cfg::kF3ConeApexY, guardPos_.z);

    renderer.setDepthWrite(false);
    renderer.drawPyramid(
        visionConeTransform(apex, guardFacingDeg_, range, width, cfg::kF3ConeHeight),
        coneColor, cfg::kF3ConeAlpha, 1.0f);
    renderer.setDepthWrite(true);
}

void StealthScene::drawNoise() {
    if (noiseTimer_ <= 0.0f) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(noisePos_.x, 0.04f, noisePos_.z),
                            glm::vec3(cfg::kF3NoiseRadius * 2.0f, 0.02f,
                                      cfg::kF3NoiseRadius * 2.0f),
                            cfg::kF3NoiseColor, 0.16f, 1.0f);
    renderer.setDepthWrite(true);
}

void StealthScene::drawAimMarker() {
    if (!aiming_ || !aimValid_) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    const glm::vec3 color = aimBlocked_ ? cfg::kF3MarkerBlocked : cfg::kF3MarkerOpen;

    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(aimImpact_.x, 0.03f, aimImpact_.z),
                            glm::vec3(1.25f, 0.02f, 1.25f),
                            color, 0.40f, 1.0f);
    renderer.setDepthWrite(true);
}

float StealthScene::px(float designPixels) const {
    return designPixels * uiScale_;
}

void StealthScene::drawSuspicionMeter() {
    TextRenderer& text = ctx().renderer.text();

    const float centerX = static_cast<float>(ctx().framebufferWidth) * 0.5f;
    const float top     = px(132.0f);

    if (caughtFlashTimer_ > 0.0f) {
        text.drawCentered("CAUGHT! BACK TO THE START", centerX, top,
                          px(cfg::kFloorNameScale * 0.75f), cfg::kF3ConeAlert);
        return;
    }

    if (suspicion_ <= 0.0f) {
        return;
    }

    const float barWidth  = px(cfg::kF3MeterWidth);
    const float barHeight = px(cfg::kF3MeterHeight);
    const float barX      = centerX - barWidth * 0.5f;

    text.fillRect(barX, top, barWidth, barHeight, cfg::kBgBottom, 0.80f);
    text.fillRect(barX, top, barWidth * std::min(suspicion_, 1.0f), barHeight,
                  cfg::kF3MeterFill, 0.95f);

    text.drawCentered(playerSeen_ ? "SPOTTED" : "LOSING INTEREST",
                      centerX, top + barHeight + px(6.0f),
                      px(cfg::kHintScale), cfg::kTextPrimary);
}

void StealthScene::drawUi() {
    TextRenderer& text = ctx().renderer.text();
    const int width  = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    text.begin(width, height);

    const float centerX = static_cast<float>(width) * 0.5f;
    const float bottom  = static_cast<float>(height);
    const float hint    = px(cfg::kHintScale);

    text.drawCentered(cfg::kFloorInfo[kFloorIndex].name, centerX, px(32.0f),
                      px(cfg::kFloorNameScale), cfg::kTextPrimary);

    text.draw("PAWS " + std::to_string(collectedCount())
                  + "/" + std::to_string(cfg::kF3ItemCount),
              px(24.0f), px(24.0f), hint, cfg::kTextPrimary);
    text.draw("ROCKS " + std::to_string(rocksLeft_)
                  + "/" + std::to_string(cfg::kF3RockCount),
              px(24.0f), px(54.0f), hint, cfg::kTextPrimary);

    drawSuspicionMeter();

    if (lockedFlashTimer_ > 0.0f) {
        text.drawCentered("THE DOOR IS LOCKED - FIND ALL THE PAWS",
                          centerX, bottom - px(160.0f), hint, cfg::kTextLocked);
    } else if (doorIsOpen()) {
        text.drawCentered("THE DOOR IS OPEN - GET OUT",
                          centerX, bottom - px(160.0f), hint, cfg::kTextComplete);
    }

    if (aiming_) {
        text.drawCentered("CLICK TO THROW     E TO CANCEL",
                          centerX, bottom - px(126.0f), hint, cfg::kTextPrimary);
    }

    text.drawCentered("WASD - MOVE     SHIFT - SNEAK     E - AIM A ROCK",
                      centerX, bottom - px(90.0f), hint, cfg::kTextPrimary);
    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f), hint, cfg::kTextDim);

    text.end();
}
