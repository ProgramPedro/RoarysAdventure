#include "scenes/TunnelScene.h"

#include "core/Camera.h"
#include "core/Config.h"
#include "core/GameProgress.h"
#include "core/Input.h"
#include "core/MascotModels.h"
#include "core/Renderer.h"
#include "core/SceneContext.h"
#include "core/SceneManager.h"
#include "scenes/TunnelLevel.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

// The tunnel runs along +Z, so that is "forward" for everything below.
constexpr glm::vec3 kForward{0.0f, 0.0f, 1.0f};

float moveToward(float value, float target, float maxDelta) {
    if (std::abs(target - value) <= maxDelta) return target;
    return value + (target > value ? maxDelta : -maxDelta);
}

// Shortest signed difference between two angles, in degrees.
float angleDifference(float from, float to) {
    float difference = to - from;
    while (difference > 180.0f)  difference -= 360.0f;
    while (difference < -180.0f) difference += 360.0f;
    return difference;
}

} // namespace

//=============================================================================
// Face geometry
//
// Walking off the right-hand edge of a face takes you to face + 1, arriving at
// its LEFT edge. That single rule is what sends you round the tunnel, and the
// corner pairings in TunnelLevel.h fall straight out of it.
//=============================================================================

glm::vec3 TunnelScene::faceDown(int face) {
    // Ordered so that face + 1 is always the face you reach by strafing
    // RIGHT on screen. Getting this order wrong is what made the controls
    // feel mirrored.
    switch (face & 3) {
        case 0:  return { 0.0f, -1.0f, 0.0f};   // floor
        case 1:  return {-1.0f,  0.0f, 0.0f};   // the wall on your right
        case 2:  return { 0.0f,  1.0f, 0.0f};   // ceiling
        default: return { 1.0f,  0.0f, 0.0f};   // the wall on your left
    }
}

glm::vec3 TunnelScene::faceUp(int face) {
    return -faceDown(face);
}

glm::vec3 TunnelScene::faceRight(int face) {
    // right = forward x up. With the camera looking down +Z this is the
    // direction that appears to the right ON SCREEN, which is what the
    // strafe keys have to agree with. The other way round (up x forward)
    // points the opposite way and the controls come out mirrored.
    return glm::cross(kForward, faceUp(face));
}

glm::vec3 TunnelScene::worldPosition() const {
    return faceDown(face_) * cfg::tunnel::halfSide()
         + faceUp(face_) * height_
         + faceRight(face_) * lateral_
         + kForward * distance_;
}

int TunnelScene::currentSegment() const {
    return static_cast<int>(std::floor(distance_ / cfg::tunnel::kSegmentLength));
}

int TunnelScene::currentLane() const {
    const float fromEdge = lateral_ + cfg::tunnel::halfSide();
    const int lane = static_cast<int>(std::floor(fromEdge / cfg::tunnel::laneWidth()));
    return std::clamp(lane, 0, cfg::tunnel::kLanesPerFace - 1);
}

bool TunnelScene::tileUnderFoot() const {
    return level2::hasTile(currentSegment(), face_, currentLane());
}

//=============================================================================
// Lifecycle
//=============================================================================

bool TunnelScene::init() {
    face_ = 0;
    lateral_ = 0.0f;
    height_ = 0.0f;
    distance_ = 0.0f;
    lateralVelocity_ = 0.0f;
    upVelocity_ = 0.0f;
    forwardSpeed_ = cfg::tunnel::kStartSpeed;
    grounded_ = true;
    coyoteTimer_ = 0.0f;

    rollDegrees_ = 0.0f;
    elapsedTime_ = 0.0f;
    wipeouts_ = 0;
    finished_ = false;
    finishHoldTimer_ = 0.0f;
    return true;
}

void TunnelScene::restartLevel() {
    // Miss a jump and the whole run starts again, the way the games this is
    // based on do it. The tunnel is short enough that a restart is a setback
    // rather than a punishment, and it keeps the pressure on.
    face_ = 0;
    lateral_ = 0.0f;
    height_ = 0.0f;
    lateralVelocity_ = 0.0f;
    upVelocity_ = 0.0f;
    distance_ = 0.0f;
    forwardSpeed_ = cfg::tunnel::kStartSpeed;
    grounded_ = true;
    rollDegrees_ = 0.0f;
    ++wipeouts_;
}

//=============================================================================
// Input
//=============================================================================

void TunnelScene::handleInput(const Input& input) {
    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        ctx().manager.requestScene(SceneId::LevelSelect);
        return;
    }
    if (input.wasPressed(GLFW_KEY_C)) {
        std::cout << "[Floor 2] DEBUG complete at " << static_cast<int>(distance_)
                  << "m, " << wipeouts_ << " wipeout(s)\n";
        ctx().progress.markCompleted(1);
        ctx().manager.requestScene(SceneId::LevelSelect);
        return;
    }
    if (finished_) {
        strafeInput_ = 0.0f;
        return;
    }
    if (input.wasPressed(GLFW_KEY_R)) {
        restartLevel();
    }

    strafeInput_ = 0.0f;
    if (input.isDown(GLFW_KEY_A) || input.isDown(GLFW_KEY_LEFT))  strafeInput_ -= 1.0f;
    if (input.isDown(GLFW_KEY_D) || input.isDown(GLFW_KEY_RIGHT)) strafeInput_ += 1.0f;

    if (input.wasPressed(GLFW_KEY_SPACE) || input.wasPressed(GLFW_KEY_W) ||
        input.wasPressed(GLFW_KEY_UP)) {
        jumpQueued_ = true;
    }
}

//=============================================================================
// Update
//=============================================================================

void TunnelScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;

    if (finished_) {
        finishHoldTimer_ -= deltaTime;
        if (finishHoldTimer_ <= 0.0f) {
            std::cout << "[Floor 2] RAN THE WHOLE TUNNEL - "
                      << wipeouts_ << " wipeout(s)\n";
            ctx().progress.markCompleted(1);
            ctx().manager.requestScene(SceneId::LevelSelect);
        }
        updateRoll(deltaTime);
        return;
    }

    updateRun(deltaTime);
    updateRoll(deltaTime);
}

void TunnelScene::updateRun(float deltaTime) {
    using namespace cfg::tunnel;

    // --- forward: you never control this, it only gets faster --------------
    forwardSpeed_ = std::min(kMaxSpeed, forwardSpeed_ + kSpeedRamp * deltaTime);
    distance_ += forwardSpeed_ * deltaTime;

    if (currentSegment() >= level2::kSegmentCount - 1) {
        finished_ = true;
        finishHoldTimer_ = 1.5f;
        return;
    }

    // --- strafing across the face ------------------------------------------
    const float targetLateral = strafeInput_ * kStrafeSpeed;
    lateralVelocity_ = moveToward(lateralVelocity_, targetLateral, kStrafeAccel * deltaTime);
    lateral_ += lateralVelocity_ * deltaTime;

    // --- rounding a corner ---------------------------------------------------
    // Running past the edge of a face puts you on the neighbour, arriving at
    // its opposite edge. Carrying the overshoot across means no stutter at the
    // seam, and because the face is an integer this can never drift.
    const float half = halfSide();
    if (lateral_ > half) {
        face_ = (face_ + 1) & 3;
        lateral_ = -half + (lateral_ - half);
    } else if (lateral_ < -half) {
        face_ = (face_ + 3) & 3;
        lateral_ = half + (lateral_ + half);
    }

    // --- jumping -------------------------------------------------------------
    if (jumpQueued_ && (grounded_ || coyoteTimer_ > 0.0f)) {
        upVelocity_ = jumpSpeed();
        grounded_ = false;
        coyoteTimer_ = 0.0f;
    }
    jumpQueued_ = false;

    // --- gravity, pulling toward whichever face is currently "down" --------
    const float gravityScale = (upVelocity_ < 0.0f) ? kFallGravityMultiplier : 1.0f;
    upVelocity_ -= gravity() * gravityScale * deltaTime;
    height_ += upVelocity_ * deltaTime;

    // --- landing --------------------------------------------------------------
    const bool wasGrounded = grounded_;
    if (height_ <= 0.0f && upVelocity_ <= 0.0f && tileUnderFoot()) {
        height_ = 0.0f;
        upVelocity_ = 0.0f;
        grounded_ = true;
    } else if (height_ > 0.0f) {
        grounded_ = false;
    } else {
        // Below the surface with no tile beneath: dropping through a hole.
        grounded_ = false;
    }

    if (wasGrounded && !grounded_) {
        coyoteTimer_ = kCoyoteTime;
    } else if (coyoteTimer_ > 0.0f) {
        coyoteTimer_ = std::max(0.0f, coyoteTimer_ - deltaTime);
    }

    // --- missed it ------------------------------------------------------------
    if (height_ < -kFallLimit) {
        restartLevel();
        return;
    }

}

void TunnelScene::updateRoll(float deltaTime) {
    // The view chases the face you are on. This is the only part of the
    // orientation that interpolates; the face itself is an integer.
    const float target = static_cast<float>(face_) * 90.0f;
    const float difference = angleDifference(rollDegrees_, target);
    const float step = cfg::tunnel::kRollSpeed * deltaTime;

    if (std::abs(difference) <= step) {
        rollDegrees_ = target;
    } else {
        rollDegrees_ += (difference > 0.0f ? step : -step);
    }
}

//=============================================================================
// Render
//=============================================================================

void TunnelScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;
    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    renderer.drawBackground(cfg::tunnel::kVoidColor,
                            glm::vec3(0.0f, 0.0f, 0.0f),
                            cfg::kF2RimGlow, glm::vec2(0.0f, 0.0f), 0.9f, 0.12f);

    // The camera's up vector is rolled to match the face, which is what makes
    // the tunnel appear to rotate around the player rather than the other way.
    // Rolling by +90 degrees per face must land on faceUp(1), faceUp(2), ...
    // in that order, which is what this sign gives.
    const float roll = glm::radians(rollDegrees_);
    const glm::vec3 up(std::sin(roll), std::cos(roll), 0.0f);

    const glm::vec3 player = worldPosition();
    Camera camera(player - kForward * cfg::tunnel::kCameraBack + up * cfg::tunnel::kCameraUp,
                  player + kForward * cfg::tunnel::kCameraAhead);
    camera.setUp(up);
    camera.setFovDegrees(cfg::tunnel::kCameraFovDeg);
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawTunnel();

    // Roary, rolled onto whichever surface he is running along.
    mascots::drawRoary(renderer, player, cfg::tunnel::kPlayerSize.y, 0.0f, -rollDegrees_);

    drawHud();
}

void TunnelScene::drawTunnel() {
    Renderer& renderer = ctx().renderer;
    using namespace cfg::tunnel;

    const int here = currentSegment();
    const int first = std::max(0, here - 3);
    const int last = std::min(level2::kSegmentCount - 1, here + kDrawDistance);

    for (int segment = first; segment <= last; ++segment) {
        // Tiles dim with distance so the tunnel recedes into the dark rather
        // than ending in a hard edge.
        const float away = static_cast<float>(segment - here) / static_cast<float>(kDrawDistance);
        const float fade = std::clamp(1.0f - away * away, 0.08f, 1.0f);

        const float z = (static_cast<float>(segment) + 0.5f) * kSegmentLength;

        for (int face = 0; face < 4; ++face) {
            const glm::vec3 down = faceDown(face);
            const glm::vec3 right = faceRight(face);
            const bool horizontal = (face == 0 || face == 2);

            for (int lane = 0; lane < kLanesPerFace; ++lane) {
                if (!level2::hasTile(segment, face, lane)) continue;

                const float laneCenter = -halfSide()
                    + (static_cast<float>(lane) + 0.5f) * laneWidth();

                const glm::vec3 center = down * halfSide()
                                       + down * (kTileThickness * 0.5f)
                                       + right * laneCenter
                                       + kForward * z;

                const glm::vec3 size = horizontal
                    ? glm::vec3(laneWidth(), kTileThickness, kSegmentLength)
                    : glm::vec3(kTileThickness, laneWidth(), kSegmentLength);

                // Alternate the two accent colors so the tiles have a rhythm
                // you can read at speed.
                const glm::vec3 accent = ((segment + lane) % 2 == 0) ? kTileTopA : kTileTopB;

                renderer.drawBoxAt(center, size, kTileBody * fade);

                // A bright inner face: the surface you actually run on. It is
                // pushed slightly PROUD of the tile rather than flush with it -
                // flush would make the two coplanar and they would z-fight into
                // a mess of stripes at distance.
                const glm::vec3 capCenter = center - down * (kTileThickness * 0.54f);
                const glm::vec3 capSize = horizontal
                    ? glm::vec3(laneWidth() * 0.94f, kTileThickness * 0.16f, kSegmentLength * 0.94f)
                    : glm::vec3(kTileThickness * 0.16f, laneWidth() * 0.94f, kSegmentLength * 0.94f);

                renderer.drawBoxAt(capCenter, capSize, accent * fade, 1.0f, 0.55f);
            }
        }
    }

    // The finish: a ring of light across the end of the tunnel.
    const float goalZ = static_cast<float>(level2::kSegmentCount) * kSegmentLength;
    if (here > level2::kSegmentCount - kDrawDistance) {
        const float pulse = 0.6f + 0.4f * std::sin(elapsedTime_ * 4.0f);
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 down = faceDown(i);
            const glm::vec3 right = faceRight(i);
            renderer.drawBoxAt(down * (halfSide() * 0.92f) + kForward * goalZ,
                               glm::abs(right) * kTunnelSide + glm::abs(down) * 0.3f
                                   + glm::vec3(0.0f, 0.0f, 0.3f),
                               kGoalRing, pulse, 1.0f);
        }
    }
}

void TunnelScene::drawHud() {
    Renderer& renderer = ctx().renderer;
    TextRenderer& text = renderer.text();

    const float width = static_cast<float>(ctx().framebufferWidth);
    const float height = static_cast<float>(ctx().framebufferHeight);
    const float centerX = width * 0.5f;
    const auto px = [this](float design) { return design * uiScale_; };

    text.begin(ctx().framebufferWidth, ctx().framebufferHeight);

    const cfg::FloorInfo& info = cfg::kFloorInfo[1];
    text.drawCentered(info.name, centerX, px(26.0f), px(cfg::kFloorNameScale), cfg::kTextPrimary);
    text.drawCentered(info.description, centerX, px(72.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    // Progress bar: the clearest way to show how far down a tunnel you are.
    const float total = static_cast<float>(level2::kSegmentCount) * cfg::tunnel::kSegmentLength;
    const float progress = std::clamp(distance_ / total, 0.0f, 1.0f);

    const float barWidth = width * 0.44f;
    const float barX = centerX - barWidth * 0.5f;
    const float barY = px(112.0f);

    text.fillRect(barX, barY, barWidth, px(10.0f), cfg::kBgBottom, 0.85f);
    text.fillRect(barX, barY, barWidth * progress, px(10.0f), cfg::kF2Accent, 0.95f);

    const std::string distanceLabel =
        std::to_string(static_cast<int>(distance_)) + " / " +
        std::to_string(static_cast<int>(total)) + " M";
    text.draw(distanceLabel,
              width - px(24.0f) - TextRenderer::measureWidth(distanceLabel, px(3.0f)),
              px(26.0f), px(3.0f), cfg::kF2Accent);

    const std::string wipeLabel = "WIPEOUTS " + std::to_string(wipeouts_);
    text.draw(wipeLabel,
              width - px(24.0f) - TextRenderer::measureWidth(wipeLabel, px(cfg::kHintScale)),
              px(62.0f), px(cfg::kHintScale), cfg::kTextDim);

    if (finished_) {
        text.drawCentered("TUNNEL CLEARED!", centerX, height * 0.42f,
                          px(5.0f), cfg::kTextComplete);
    }

    text.drawCentered("A/D - STRAFE     SPACE - JUMP     R - RESTART",
                      centerX, height - px(78.0f), px(cfg::kHintScale), cfg::kTextPrimary);
    text.drawCentered("RUN ONTO A WALL AND IT BECOMES THE FLOOR",
                      centerX, height - px(48.0f), px(cfg::kHintScale), cfg::kFiuGold);

    text.end();
}
