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

char TunnelScene::tileHere() const {
    return level2::tileAt(currentSegment(), face_, currentLane());
}

int TunnelScene::crumbleIndex(int segment, int face, int lane) const {
    if (segment < 0 || segment >= level2::kSegmentCount) return -1;
    if (face < 0 || face > 3 || lane < 0 || lane > 2) return -1;
    return (segment * 4 + face) * 3 + lane;
}

bool TunnelScene::crumbledAway(int segment, int face, int lane) const {
    const int index = crumbleIndex(segment, face, lane);
    if (index < 0) return false;
    const float age = crumble_[static_cast<std::size_t>(index)];
    return age >= 0.0f && age >= cfg::tunnel::kCrumbleDelay;
}

bool TunnelScene::solidAt(int segment, int face, int lane) const {
    const char tile = level2::tileAt(segment, face, lane);
    if (tile == level2::kEmpty) return false;
    if (tile == level2::kCrumble && crumbledAway(segment, face, lane)) return false;
    return true;
}

bool TunnelScene::tileUnderFoot() const {
    return solidAt(currentSegment(), face_, currentLane());
}

//=============================================================================
// Lifecycle
//=============================================================================

bool TunnelScene::init() {
    face_ = 0;
    lateral_ = 0.0f;
    height_ = 0.0f;
    // --dist on the command line starts you partway down, for playtesting a
    // late section without running the whole tunnel first. A restart goes
    // back to the same place, so a tester can keep retrying that one part.
    startDistance_ = ctx().debugStart.overrideDistance
                   ? ctx().debugStart.distance
                   : 0.0f;
    distance_ = startDistance_;
    lateralVelocity_ = 0.0f;
    upVelocity_ = 0.0f;
    rampSpeed_ = cfg::tunnel::kStartSpeed;
    boost_ = 0.0f;
    forwardSpeed_ = rampSpeed_;
    grounded_ = true;
    coyoteTimer_ = 0.0f;

    // One crumble slot per tile position. Negative means untouched.
    crumble_.assign(static_cast<std::size_t>(level2::kSegmentCount * 4 * 3), -1.0f);
    checkpointSegment_ = static_cast<int>(startDistance_ / cfg::tunnel::kSegmentLength);

    rollDegrees_ = 0.0f;
    elapsedTime_ = 0.0f;
    wipeouts_ = 0;
    finished_ = false;
    finishHoldTimer_ = 0.0f;
    return true;
}

void TunnelScene::restartLevel() {
    // Miss a jump and the run starts again, the way the games this is based on
    // do it. Where "again" means is cfg::tunnel::kUseCheckpoints: off, it is
    // the very beginning (or wherever --dist put you); on, it is the start of
    // the act you were in, listed in TunnelLevel.h.
    float resumeAt = startDistance_;
    if (cfg::tunnel::kUseCheckpoints) {
        const float checkpoint =
            static_cast<float>(checkpointSegment_) * cfg::tunnel::kSegmentLength;
        resumeAt = std::max(startDistance_, checkpoint);
    }

    face_ = 0;
    lateral_ = 0.0f;
    height_ = 0.0f;
    lateralVelocity_ = 0.0f;
    upVelocity_ = 0.0f;
    distance_ = resumeAt;
    rampSpeed_ = cfg::tunnel::kStartSpeed;
    boost_ = 0.0f;
    forwardSpeed_ = rampSpeed_;
    grounded_ = true;
    rollDegrees_ = 0.0f;

    // Crumbled tiles come back, otherwise a retry would be harder than the
    // first attempt and the level would get less fair the more you practised.
    crumble_.assign(crumble_.size(), -1.0f);

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

    // --- forward: you never steer this, it only gets faster ----------------
    // The ramp creeps up for the whole run; the boost is whatever a speed
    // tile last gave you, bleeding away. Adding them keeps a boost temporary.
    rampSpeed_ = std::min(kMaxSpeed, rampSpeed_ + kSpeedRamp * deltaTime);
    boost_ = std::max(0.0f, boost_ - kBoostDecay * deltaTime);
    forwardSpeed_ = rampSpeed_ + boost_;
    distance_ += forwardSpeed_ * deltaTime;

    if (currentSegment() >= level2::kSegmentCount - 1) {
        finished_ = true;
        finishHoldTimer_ = 1.5f;
        return;
    }

    // Crumbling tiles that have been stood on keep counting down whether or
    // not you are still on them.
    for (float& age : crumble_) {
        if (age >= 0.0f) age += deltaTime;
    }

    // --- strafing across the face ------------------------------------------
    // Ice is simply much less steering force. You keep whatever sideways
    // speed you already had, which is exactly what makes it feel slippery.
    const bool onIce = grounded_ && tileHere() == level2::kIce;
    const float grip = onIce ? kStrafeAccel * kIceGrip : kStrafeAccel;

    const float targetLateral = strafeInput_ * kStrafeSpeed;
    lateralVelocity_ = moveToward(lateralVelocity_, targetLateral, grip * deltaTime);
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
    // The window has to be at least as deep as one frame of falling, or a fast
    // descent steps straight past a tile and through the floor. It also has to
    // be BOUNDED: without a lower limit, a player who has already fallen five
    // units down a gap gets snapped back up by the next tile along, and the
    // long gaps in act 5 would stop being gaps at all.
    const bool wasGrounded = grounded_;
    const float landWindow = std::max(0.40f, std::abs(upVelocity_) * deltaTime * 1.5f);

    if (height_ <= 0.0f && height_ >= -landWindow &&
        upVelocity_ <= 0.0f && tileUnderFoot()) {
        height_ = 0.0f;
        upVelocity_ = 0.0f;
        grounded_ = true;
    } else {
        grounded_ = false;
    }

    if (wasGrounded && !grounded_) {
        coyoteTimer_ = kCoyoteTime;
    } else if (coyoteTimer_ > 0.0f) {
        coyoteTimer_ = std::max(0.0f, coyoteTimer_ - deltaTime);
    }

    // --- what the tile you are on does to you ---------------------------------
    applyTileEffects(deltaTime);

    // --- walked into a crate --------------------------------------------------
    // A crate stands on its tile, so you are clear of it only once your feet
    // are above its top. Checked after the landing resolution so that landing
    // ON a crate counts as hitting it, which is what it looks like.
    if (tileHere() == level2::kBlock && height_ < kBlockHeight) {
        restartLevel();
        return;
    }

    // --- missed it ------------------------------------------------------------
    if (height_ < -kFallLimit) {
        restartLevel();
        return;
    }

    // --- passed a checkpoint --------------------------------------------------
    checkpointSegment_ = std::max(checkpointSegment_,
                                  level2::checkpointBefore(currentSegment()));
}

//-----------------------------------------------------------------------------
// The special tiles, all of which only do anything while you are standing on
// one. Keeping them in one place means adding a tile type is one case here
// plus one case in drawTunnel.
//-----------------------------------------------------------------------------
void TunnelScene::applyTileEffects(float deltaTime) {
    using namespace cfg::tunnel;
    (void)deltaTime;

    if (!grounded_) return;

    const int segment = currentSegment();
    const int lane = currentLane();

    switch (level2::tileAt(segment, face_, lane)) {
    case level2::kCrumble: {
        // Start the clock the first time anyone stands here. It keeps running
        // after you leave, so doubling back onto it does not help.
        const int index = crumbleIndex(segment, face_, lane);
        if (index >= 0 && crumble_[static_cast<std::size_t>(index)] < 0.0f) {
            crumble_[static_cast<std::size_t>(index)] = 0.0f;
        }
        break;
    }
    case level2::kSpring:
        // Fires immediately and takes your feet off the tile, so it cannot
        // retrigger on the same frame.
        upVelocity_ = springSpeed();
        height_ = 0.01f;
        grounded_ = false;
        coyoteTimer_ = 0.0f;
        break;

    case level2::kSpeed:
        // Set rather than accumulate: running along a row of speed tiles
        // should feel like one shove, not like it is stacking up.
        boost_ = kSpeedTileBoost;
        break;

    default:
        break;
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
            const glm::vec3 up = faceUp(face);
            const glm::vec3 right = faceRight(face);
            const bool horizontal = (face == 0 || face == 2);

            for (int lane = 0; lane < kLanesPerFace; ++lane) {
                const char tile = level2::tileAt(segment, face, lane);
                if (tile == level2::kEmpty) continue;

                // A crumbling tile that has already gone is simply not drawn.
                if (tile == level2::kCrumble && crumbledAway(segment, face, lane)) continue;

                const float laneCenter = -halfSide()
                    + (static_cast<float>(lane) + 0.5f) * laneWidth();

                // --- a crumbling tile rattles, then drops --------------------
                // Reading the warning is the whole skill, so it has to be
                // visible: it shakes sideways and sinks as its time runs out.
                glm::vec3 wobble(0.0f);
                float tileAlpha = 1.0f;
                if (tile == level2::kCrumble) {
                    const int index = crumbleIndex(segment, face, lane);
                    const float age = index >= 0
                        ? crumble_[static_cast<std::size_t>(index)] : -1.0f;
                    if (age >= 0.0f) {
                        const float t = std::clamp(age / kCrumbleDelay, 0.0f, 1.0f);
                        wobble = right * (std::sin(age * 58.0f) * kCrumbleShake)
                               + down * (t * t * 0.55f);
                        tileAlpha = 1.0f - t * 0.45f;
                    }
                }

                const glm::vec3 surface = down * halfSide() + right * laneCenter
                                        + kForward * z + wobble;
                const glm::vec3 center = surface + down * (kTileThickness * 0.5f);

                const glm::vec3 size = horizontal
                    ? glm::vec3(laneWidth(), kTileThickness, kSegmentLength)
                    : glm::vec3(kTileThickness, laneWidth(), kSegmentLength);

                // --- the colour of the running surface says what the tile is --
                // Colour is the only warning a player gets at this speed, so
                // every type is a clearly different hue, not a shade.
                glm::vec3 accent;
                float capGlow = 0.55f;
                switch (tile) {
                case level2::kCrumble: accent = kCrumbleColor; capGlow = 0.70f; break;
                case level2::kSpring:  accent = kSpringColor;  capGlow = 0.85f; break;
                case level2::kSpeed:   accent = kSpeedColor;   capGlow = 0.85f; break;
                case level2::kIce:     accent = kIceColor;     capGlow = 0.80f; break;
                default:
                    // Plain tiles (and the tile under a crate) alternate the
                    // two house colours so the run has a readable rhythm.
                    accent = ((segment + lane) % 2 == 0) ? kTileTopA : kTileTopB;
                    break;
                }

                renderer.drawBoxAt(center, size, kTileBody * fade, tileAlpha);

                // A bright inner face: the surface you actually run on. It is
                // pushed slightly PROUD of the tile rather than flush with it -
                // flush would make the two coplanar and they would z-fight into
                // a mess of stripes at distance.
                const glm::vec3 capCenter = center - down * (kTileThickness * 0.54f);
                const glm::vec3 capSize = horizontal
                    ? glm::vec3(laneWidth() * 0.94f, kTileThickness * 0.16f, kSegmentLength * 0.94f)
                    : glm::vec3(kTileThickness * 0.16f, laneWidth() * 0.94f, kSegmentLength * 0.94f);

                renderer.drawBoxAt(capCenter, capSize, accent * fade, tileAlpha, capGlow);

                // --- the bits that stand proud of the surface ----------------
                if (tile == level2::kBlock) {
                    // The crate. Narrower than its lane so you can see the gap
                    // either side of it and judge whether a strafe will clear.
                    const glm::vec3 crateSize = horizontal
                        ? glm::vec3(laneWidth() * 0.62f, kBlockHeight, kSegmentLength * 0.58f)
                        : glm::vec3(kBlockHeight, laneWidth() * 0.62f, kSegmentLength * 0.58f);

                    renderer.drawBoxAt(surface + up * (kBlockHeight * 0.5f),
                                       crateSize, kBlockColor * fade, 1.0f, 0.30f);

                    // A pale band round the top edge, so the height you have
                    // to clear reads at a glance.
                    const glm::vec3 bandSize = horizontal
                        ? glm::vec3(crateSize.x * 1.06f, kBlockHeight * 0.12f, crateSize.z * 1.06f)
                        : glm::vec3(kBlockHeight * 0.12f, crateSize.y * 1.06f, crateSize.z * 1.06f);

                    renderer.drawBoxAt(surface + up * (kBlockHeight * 0.92f),
                                       bandSize, glm::vec3(1.0f) * fade, 1.0f, 0.55f);

                } else if (tile == level2::kSpring) {
                    // A low pad with a pulse in it, so it reads as a thing
                    // that will do something to you rather than just a colour.
                    const float pulse = 0.55f + 0.45f * std::sin(elapsedTime_ * 7.0f
                                                                 + static_cast<float>(segment));
                    const glm::vec3 padSize = horizontal
                        ? glm::vec3(laneWidth() * 0.70f, 0.22f, kSegmentLength * 0.62f)
                        : glm::vec3(0.22f, laneWidth() * 0.70f, kSegmentLength * 0.62f);

                    renderer.drawBoxAt(surface + up * 0.11f, padSize,
                                       kSpringColor * fade, 1.0f, 0.45f + 0.45f * pulse);

                } else if (tile == level2::kSpeed) {
                    // Two chevron bars pointing down the tunnel.
                    for (int bar = 0; bar < 2; ++bar) {
                        const float offset = (static_cast<float>(bar) - 0.5f) * kSegmentLength * 0.42f;
                        const glm::vec3 barSize = horizontal
                            ? glm::vec3(laneWidth() * 0.60f, 0.10f, kSegmentLength * 0.14f)
                            : glm::vec3(0.10f, laneWidth() * 0.60f, kSegmentLength * 0.14f);

                        renderer.drawBoxAt(surface + up * 0.07f + kForward * offset,
                                           barSize, glm::vec3(1.0f) * fade, 1.0f, 0.9f);
                    }
                }
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

    // Which act you are in. The tunnel is long enough now that "how far am I"
    // needs an answer in words, not just a bar - and naming the section also
    // tells you what the next hazard is likely to be.
    if (!finished_) {
        const std::string act = level2::actNameFor(currentSegment());
        text.drawCentered(act, centerX, px(130.0f), px(2.4f), cfg::kFiuGold, 0.85f);
    }

    // A speed tile's boost, while it lasts. Without this the sudden change in
    // pace reads as the game glitching rather than as something you ran over.
    if (boost_ > 0.05f) {
        const float strength = boost_ / cfg::tunnel::kSpeedTileBoost;
        const std::string boostLabel = "BOOST";
        text.draw(boostLabel, px(24.0f), px(26.0f), px(3.0f),
                  cfg::tunnel::kSpeedColor, strength);
        text.fillRect(px(24.0f), px(50.0f), px(90.0f) * strength, px(8.0f),
                      cfg::tunnel::kSpeedColor, strength);
    }

    if (finished_) {
        text.drawCentered("TUNNEL CLEARED!", centerX, height * 0.42f,
                          px(5.0f), cfg::kTextComplete);
    }

    // A dark band under the hints. Without it the text sits straight on top
    // of the tunnel and the brightest tiles make it unreadable.
    text.fillRect(0.0f, height - px(96.0f), width, px(96.0f), cfg::kBgBottom, 0.72f);

    text.drawCentered("A/D - STRAFE     SPACE - JUMP     R - RESTART",
                      centerX, height - px(82.0f), px(cfg::kHintScale), cfg::kTextPrimary);
    text.drawCentered("RUN ONTO A WALL AND IT BECOMES THE FLOOR",
                      centerX, height - px(55.0f), px(cfg::kHintScale), cfg::kFiuGold);

    // The tile legend. Five types is more than anyone will remember from one
    // playthrough, so the colours are spelled out rather than discovered.
    const float legendY = height - px(28.0f);
    const struct { const char* label; glm::vec3 color; } legend[] = {
        {"CRATE - DODGE",   cfg::tunnel::kBlockColor  },
        {"CRUMBLES",        cfg::tunnel::kCrumbleColor},
        {"SPRING",          cfg::tunnel::kSpringColor },
        {"SPEED",           cfg::tunnel::kSpeedColor  },
        {"ICE",             cfg::tunnel::kIceColor    },
    };

    float legendWidth = 0.0f;
    for (const auto& entry : legend) {
        legendWidth += TextRenderer::measureWidth(entry.label, px(cfg::kHintScale)) + px(26.0f);
    }
    float legendX = centerX - legendWidth * 0.5f;
    for (const auto& entry : legend) {
        text.draw(entry.label, legendX, legendY, px(cfg::kHintScale), entry.color, 0.85f);
        legendX += TextRenderer::measureWidth(entry.label, px(cfg::kHintScale)) + px(26.0f);
    }

    text.end();
}
