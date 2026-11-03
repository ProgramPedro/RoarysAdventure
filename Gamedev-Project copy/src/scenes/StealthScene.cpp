#include "scenes/StealthScene.h"

#include "core/Camera.h"
#include "core/Config.h"
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
#include <utility>

namespace {

// Shortest signed difference between two angles in degrees, in -180..180.
// Without this, turning from 350 degrees to 10 takes the long way round and
// Roary spins almost all the way through a circle to face forward.
float shortestAngleDelta(float fromDeg, float toDeg) {
    float delta = std::fmod(toDeg - fromDeg + 540.0f, 360.0f) - 180.0f;
    return delta;
}


// The vertical FOV that keeps the HORIZONTAL field of view fixed at the design
// aspect ratio. Widening the window just reveals more to the sides; narrowing
// it adds height rather than slicing the ends off a room that is 32 units wide.
float fovForAspect(float baseFovDeg, float aspect) {
    if (aspect >= cfg::kF3DesignAspect || aspect <= 0.0f) {
        return baseFovDeg;
    }

    // Horizontal half-extent one unit from the camera, at the design aspect.
    const float halfWidth =
        std::tan(glm::radians(baseFovDeg * 0.5f)) * cfg::kF3DesignAspect;

    return glm::degrees(2.0f * std::atan(halfWidth / aspect));
}

// A wedge whose APEX sits at `apex` and whose body runs `range` units along
// `facingDeg`. Built by hand rather than through makeTransform because the
// pyramid primitive points along +Y and has to be tipped over and shifted so
// it grows forward from its tip instead of around its centre. Same idea as
// hangingTransform() in LevelSelectScene, one axis over.
glm::mat4 visionConeTransform(const glm::vec3& apex, float facingDeg,
                              float range, float width, float thickness) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, apex);

    // Aim it, then tip the pyramid so its local -Y (the direction the body
    // extends once it is shifted) points along world +Z before the aiming
    // rotation takes effect.
    model = glm::rotate(model, glm::radians(facingDeg), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));

    // Shift back by half the length so the TIP lands on the apex rather than
    // the middle of the wedge.
    model = glm::translate(model, glm::vec3(0.0f, -range * 0.5f, 0.0f));

    // After the tip, local Y is the cone's length and local Z is its height.
    model = glm::scale(model, glm::vec3(width, range, thickness));
    return model;
}

} // namespace

bool StealthScene::init() {
    // Clamped rather than assigned straight through, so a start position
    // edited into Config.h can never drop him inside a wall.
    playerPos_   = clampToRoom(cfg::kF3StartPos, cfg::kF3PlayerRadius);
    moveDir_     = glm::vec2(0.0f);
    facingDeg_   = 180.0f;   // looking toward the camera, into the room
    walkCycle_   = 0.0f;
    sneaking_    = false;
    elapsedTime_ = 0.0f;

    guardPos_        = cfg::kF3GuardStart;
    guardFacingDeg_  = cfg::kF3GuardFacing;

    itemCollected_.fill(false);
    playerSeen_ = false;

    return true;
}

void StealthScene::returnToMenu() {
    // Only a REQUEST: this scene keeps running for the rest of the frame and
    // SceneManager swaps it out afterwards. See SceneManager.h.
    ctx().manager.requestScene(SceneId::LevelSelect);
}

//=============================================================================
// Input
//=============================================================================

void StealthScene::handleInput(const Input& input) {
    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        returnToMenu();
        return;
    }

    // DEBUG until step 5 gives the door a real win condition.
    if (input.wasPressed(GLFW_KEY_C)) {
        ctx().progress.markCompleted(kFloorIndex);
        returnToMenu();
        return;
    }

    // Movement is isDown(), not wasPressed(): we want it every frame the key is
    // held. wasPressed() is for menus and one-shot actions like throwing.
    glm::vec2 direction(0.0f);

    if (input.isDown(GLFW_KEY_W) || input.isDown(GLFW_KEY_UP))    direction.y -= 1.0f;
    if (input.isDown(GLFW_KEY_S) || input.isDown(GLFW_KEY_DOWN))  direction.y += 1.0f;
    if (input.isDown(GLFW_KEY_A) || input.isDown(GLFW_KEY_LEFT))  direction.x -= 1.0f;
    if (input.isDown(GLFW_KEY_D) || input.isDown(GLFW_KEY_RIGHT)) direction.x += 1.0f;

    // Normalize, or holding W and D would move him 1.41x faster than holding
    // either one alone - the classic diagonal speed bug.
    const float length = glm::length(direction);
    moveDir_ = (length > 0.0f) ? direction / length : glm::vec2(0.0f);

    sneaking_ = input.isDown(GLFW_KEY_LEFT_SHIFT)
             || input.isDown(GLFW_KEY_RIGHT_SHIFT);
}

//=============================================================================
// Simulation
//=============================================================================

glm::vec3 StealthScene::clampToRoom(const glm::vec3& position, float radius) const {
    // The walls are solid boxes sitting just outside the room, so the inside
    // face of each one is at exactly +/-kF3RoomHalf*. Backing that off by the
    // radius keeps Roary's whole body inside rather than his centre point.
    const float limitX = cfg::kF3RoomHalfX - radius;
    const float limitZ = cfg::kF3RoomHalfZ - radius;

    return glm::vec3(std::clamp(position.x, -limitX, limitX),
                     position.y,
                     std::clamp(position.z, -limitZ, limitZ));
}

int StealthScene::collectedCount() const {
    int count = 0;
    for (const bool collected : itemCollected_) {
        if (collected) {
            ++count;
        }
    }
    return count;
}

void StealthScene::collectNearbyItems() {
    // Compared squared, so there is no square root per item per frame.
    const float radiusSquared = cfg::kF3ItemPickupRadius * cfg::kF3ItemPickupRadius;

    for (int i = 0; i < cfg::kF3ItemCount; ++i) {
        if (itemCollected_[i]) {
            continue;
        }

        const glm::vec3& item = cfg::kF3ItemPositions[i];
        const float dx = item.x - playerPos_.x;
        const float dz = item.z - playerPos_.z;

        // Distance on the floor only. Roary's Y never changes, and the item
        // hovers, so including Y would make the hover height part of the
        // pickup range by accident.
        if (dx * dx + dz * dz <= radiusSquared) {
            itemCollected_[i] = true;
        }
    }
}

bool StealthScene::lineCrossesWall(const glm::vec3& from, const glm::vec3& to) {
    // The classic slab test, in 2D. Walk the segment as a parametric line and
    // keep narrowing the interval of t where it could still be inside the box;
    // if the interval survives both axes, the segment and the box overlap.
    const float dx = to.x - from.x;
    const float dz = to.z - from.z;

    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];

        const float minX = wall.centerX - wall.sizeX * 0.5f;
        const float maxX = wall.centerX + wall.sizeX * 0.5f;
        const float minZ = wall.centerZ - wall.sizeZ * 0.5f;
        const float maxZ = wall.centerZ + wall.sizeZ * 0.5f;

        float enter = 0.0f;
        float exit  = 1.0f;
        bool  missed = false;

        // One pass per axis. Written as a loop over {x, z} so the near-zero
        // case - a segment running exactly parallel to a wall - is handled the
        // same way on both axes instead of being special-cased twice.
        const float origin[2] = {from.x, from.z};
        const float delta[2]  = {dx, dz};
        const float lower[2]  = {minX, minZ};
        const float upper[2]  = {maxX, maxZ};

        for (int axis = 0; axis < 2 && !missed; ++axis) {
            if (std::fabs(delta[axis]) < 1e-6f) {
                // Parallel to this pair of faces: it can only ever hit if it
                // already starts between them.
                if (origin[axis] < lower[axis] || origin[axis] > upper[axis]) {
                    missed = true;
                }
                continue;
            }

            float t1 = (lower[axis] - origin[axis]) / delta[axis];
            float t2 = (upper[axis] - origin[axis]) / delta[axis];
            if (t1 > t2) {
                std::swap(t1, t2);
            }

            enter = std::max(enter, t1);
            exit  = std::min(exit,  t2);

            if (enter > exit) {
                missed = true;
            }
        }

        if (!missed) {
            return true;   // this wall blocks the line
        }
    }

    return false;
}

bool StealthScene::guardCanSeePlayer() const {
    const glm::vec3 toPlayer = playerPos_ - guardPos_;
    const float distance = std::sqrt(toPlayer.x * toPlayer.x +
                                     toPlayer.z * toPlayer.z);

    // 1. Range. Cheapest test, so it goes first.
    if (distance > cfg::kF3VisionRange) {
        return false;
    }

    // Standing exactly on the guard counts as seen, and also makes the
    // normalize below undefined, so deal with it here.
    if (distance < 1e-4f) {
        return true;
    }

    // 2. Angle. Comparing cosines avoids an acos per frame, and the half-angle
    // cosine is a compile-time constant.
    const float facingRad = glm::radians(guardFacingDeg_);
    const glm::vec3 forward(std::sin(facingRad), 0.0f, std::cos(facingRad));

    const float alignment = (toPlayer.x * forward.x + toPlayer.z * forward.z) / distance;
    if (alignment < std::cos(glm::radians(cfg::kF3VisionHalfAngleDeg))) {
        return false;
    }

    // 3. Line of sight. Most expensive, and only reached by someone already
    // standing in the cone.
    return !lineCrossesWall(guardPos_, playerPos_);
}

void StealthScene::resolveInnerWalls(glm::vec3& position, int axis, float delta,
                                     float radius) const {
    // Nothing to resolve if he did not move on this axis.
    if (delta == 0.0f) {
        return;
    }

    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];

        const float minX = wall.centerX - wall.sizeX * 0.5f;
        const float maxX = wall.centerX + wall.sizeX * 0.5f;
        const float minZ = wall.centerZ - wall.sizeZ * 0.5f;
        const float maxZ = wall.centerZ + wall.sizeZ * 0.5f;

        // Roary is treated as a square of half-width `radius` rather than a
        // circle. Against axis-aligned walls the difference is invisible, and
        // it makes the push-out below exact instead of iterative.
        const bool overlapping =
            position.x + radius > minX && position.x - radius < maxX &&
            position.z + radius > minZ && position.z - radius < maxZ;

        if (!overlapping) {
            continue;
        }

        // Put him flush against the face he came in through. Which face that
        // is follows from the sign of the movement, which is why this has to
        // happen per axis and immediately after that axis moved.
        if (axis == 0) {
            position.x = (delta > 0.0f) ? minX - radius : maxX + radius;
        } else {
            position.z = (delta > 0.0f) ? minZ - radius : maxZ + radius;
        }
    }
}

void StealthScene::movePlayer(float deltaTime) {
    const bool walking = (moveDir_ != glm::vec2(0.0f));

    if (walking) {
        const float speed = sneaking_ ? cfg::kF3SneakSpeed : cfg::kF3WalkSpeed;
        const float radius = cfg::kF3PlayerRadius;

        const float deltaX = moveDir_.x * speed * deltaTime;
        const float deltaZ = moveDir_.y * speed * deltaTime;

        // One axis at a time, each resolved before the next moves. Moving both
        // and then resolving would make a corner push him out diagonally, and
        // walking into a wall at an angle would stop him dead instead of
        // sliding him along it.
        playerPos_.x += deltaX;
        resolveInnerWalls(playerPos_, 0, deltaX, radius);

        playerPos_.z += deltaZ;
        resolveInnerWalls(playerPos_, 2, deltaZ, radius);

        playerPos_ = clampToRoom(playerPos_, radius);

        // Face where he is heading. The model is built looking down +Z, so
        // atan2(x, z) is the rotation about Y that points it along the
        // movement vector.
        const float targetDeg =
            glm::degrees(std::atan2(moveDir_.x, moveDir_.y));

        // Ease rather than snap. The 0..1 clamp matters: at a low frame rate
        // kF3TurnSpeed * deltaTime can exceed 1, which would overshoot and
        // make him jitter around the target angle.
        const float turn = std::min(cfg::kF3TurnSpeed * deltaTime, 1.0f);
        facingDeg_ += shortestAngleDelta(facingDeg_, targetDeg) * turn;

        // Sneaking is a slower, shallower step than walking.
        const float cycleRate = sneaking_ ? cfg::kF3BobSpeed * 0.55f
                                          : cfg::kF3BobSpeed;
        walkCycle_ += cycleRate * deltaTime;
    }
}

void StealthScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;
    movePlayer(deltaTime);

    // Both of these run after the move, so they describe where Roary actually
    // ended this frame rather than where he started it.
    collectNearbyItems();
    playerSeen_ = guardCanSeePlayer();
}

//=============================================================================
// Drawing
//=============================================================================

void StealthScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width  = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    uiScale_ = static_cast<float>(height) / cfg::kUiReferenceHeight;

    // Near-black backdrop. Anything brighter and the dark room stops reading as
    // somewhere you would want to hide.
    renderer.drawBackground(
        glm::mix(cfg::kBgTop, cfg::kF3Wall, 0.70f),
        glm::mix(cfg::kBgBottom, cfg::kF3Wall, 0.30f),
        cfg::kF3Light,
        glm::vec2(0.0f, 0.1f),
        cfg::kBgGlowRadius,
        0.10f);

    const float aspect = static_cast<float>(width) / static_cast<float>(std::max(height, 1));

    Camera camera(cfg::kF3CameraPos, cfg::kF3CameraTarget);
    camera.setFovDegrees(fovForAspect(cfg::kF3CameraFovDeg, aspect));
    renderer.setCamera(camera.view(), camera.perspective(width, height));

    drawRoom();
    drawGuard();
    drawVisionCone();   // after the guard, so it lies over the deck
    drawItems();
    drawRoary();
    drawUi();
}

void StealthScene::drawRoom() {
    Renderer& renderer = ctx().renderer;

    // One wall slab: the wall itself, plus a lighter capping strip along its
    // top. The capping catches the light and gives the wall a readable top
    // edge against a dark backdrop, which matters a lot in an untextured scene.
    const auto drawSlab = [&](float centerX, float centerZ,
                              float sizeX, float sizeZ, float height) {
        renderer.drawBoxAt(glm::vec3(centerX, height * 0.5f, centerZ),
                           glm::vec3(sizeX, height, sizeZ),
                           cfg::kF3WallIn);

        renderer.drawBoxAt(glm::vec3(centerX, height + 0.06f, centerZ),
                           glm::vec3(sizeX + 0.10f, 0.12f, sizeZ + 0.10f),
                           cfg::kF3Trim);
    };

    // --- the deck ------------------------------------------------------------
    // Laid as a checkerboard of thin slabs. The tile count is derived from the
    // room size rather than hardcoded, so changing kF3RoomHalfX in Config.h
    // re-tiles the floor instead of leaving a bald strip at the edge.
    const int tilesX = static_cast<int>(
        std::ceil(cfg::kF3RoomHalfX * 2.0f / cfg::kF3TileSize));
    const int tilesZ = static_cast<int>(
        std::ceil(cfg::kF3RoomHalfZ * 2.0f / cfg::kF3TileSize));

    for (int ix = 0; ix < tilesX; ++ix) {
        for (int iz = 0; iz < tilesZ; ++iz) {
            // Tile centre, measured out from the far-left corner of the room.
            const float x = -cfg::kF3RoomHalfX
                          + (static_cast<float>(ix) + 0.5f) * cfg::kF3TileSize;
            const float z = -cfg::kF3RoomHalfZ
                          + (static_cast<float>(iz) + 0.5f) * cfg::kF3TileSize;

            // The last row and column are clipped to the room edge, so a room
            // size that is not a whole number of tiles still ends flush.
            const float sizeX = std::min(cfg::kF3TileSize,
                                         cfg::kF3RoomHalfX - (x - cfg::kF3TileSize * 0.5f));
            const float sizeZ = std::min(cfg::kF3TileSize,
                                         cfg::kF3RoomHalfZ - (z - cfg::kF3TileSize * 0.5f));

            const glm::vec3 color = ((ix + iz) % 2 == 0) ? cfg::kF3FloorA
                                                         : cfg::kF3FloorB;

            renderer.drawBoxAt(
                glm::vec3(x - (cfg::kF3TileSize - sizeX) * 0.5f,
                          -0.10f,
                          z - (cfg::kF3TileSize - sizeZ) * 0.5f),
                glm::vec3(sizeX, 0.20f, sizeZ),
                color);
        }
    }

    // --- the four outer walls ------------------------------------------------
    // Each sits just OUTSIDE the room bounds, so its inner face lands exactly
    // on +/-kF3RoomHalf* and matches what clampToRoom() assumes.
    const float halfX  = cfg::kF3RoomHalfX;
    const float halfZ  = cfg::kF3RoomHalfZ;
    const float thick  = cfg::kF3WallThick;
    const float height = cfg::kF3WallHeight;

    // Long walls run the full width plus the corners, so the ends meet flush.
    const float spanX = halfX * 2.0f + thick * 2.0f;
    const float spanZ = halfZ * 2.0f;

    // far (-Z): full height, and the wall the exit door will sit in.
    drawSlab(0.0f, -halfZ - thick * 0.5f, spanX, thick, height);
    // near (+Z): a low lip, so the camera can see over it.
    drawSlab(0.0f,  halfZ + thick * 0.5f, spanX, thick, cfg::kF3NearWallHeight);
    // left (-X) and right (+X): full height, framing the room.
    drawSlab(-halfX - thick * 0.5f, 0.0f, thick, spanZ, height);
    drawSlab( halfX + thick * 0.5f, 0.0f, thick, spanZ, height);

    // --- the interior walls --------------------------------------------------
    // Straight from the table in Config.h, which is the same data the
    // collision reads, so what you see is exactly what blocks you.
    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];
        drawSlab(wall.centerX, wall.centerZ, wall.sizeX, wall.sizeZ,
                 cfg::kF3InnerWallHeight);
    }
}

void StealthScene::drawGuard() {
    Renderer& renderer = ctx().renderer;

    const float bodyHeight = cfg::kF3GuardBodyHeight;
    const float headSize   = cfg::kF3GuardHeadSize;
    const float width      = cfg::kF3GuardRadius * 2.0f;

    // Blob shadow first, flat on the deck, same as Roary gets. Translucent, so
    // depth writes are off or it punches a hole in the floor behind it.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(guardPos_.x, 0.015f, guardPos_.z),
                            glm::vec3(width * 1.15f, 0.02f, width * 1.15f),
                            cfg::kF3ShadowColor, cfg::kF3ShadowAlpha, 1.0f);
    renderer.setDepthWrite(true);

    const glm::vec3 spin(0.0f, guardFacingDeg_, 0.0f);

    // Body and head: two boxes, the same shape the level-select diorama uses
    // for its guards, so the two screens agree on what a guard looks like.
    renderer.drawBox(
        makeTransform(guardPos_ + glm::vec3(0.0f, bodyHeight * 0.5f, 0.0f),
                      glm::vec3(width, bodyHeight, width * 0.78f), spin),
        cfg::kF3GuardBody);

    renderer.drawBox(
        makeTransform(guardPos_ + glm::vec3(0.0f, bodyHeight + headSize * 0.5f, 0.0f),
                      glm::vec3(headSize, headSize, headSize), spin),
        cfg::kF3GuardHead);

    // The visor, on the front of the head. It is the only bright thing on him,
    // so it reads from across the room AND shows which way he is looking -
    // which is exactly what the vision cone will be attached to.
    const float facingRad = glm::radians(guardFacingDeg_);
    const glm::vec3 forward(std::sin(facingRad), 0.0f, std::cos(facingRad));

    renderer.drawBox(
        makeTransform(guardPos_
                          + glm::vec3(0.0f, bodyHeight + headSize * 0.58f, 0.0f)
                          + forward * (headSize * 0.5f),
                      glm::vec3(headSize * 0.82f, headSize * 0.26f, 0.06f), spin),
        cfg::kF3GuardVisor, 1.0f, 1.0f);
}

void StealthScene::drawVisionCone() {
    Renderer& renderer = ctx().renderer;

    const float range = cfg::kF3VisionRange;

    // Width of the wedge where it ends. A pyramid tapers linearly from its tip,
    // so setting the far width to the cone's chord makes the drawn shape match
    // the angle the detection test actually uses.
    const float width =
        2.0f * range * std::tan(glm::radians(cfg::kF3VisionHalfAngleDeg));

    const glm::vec3 apex(guardPos_.x, cfg::kF3ConeApexY, guardPos_.z);

    // Translucent, so depth writes go off or the cone hides the floor and the
    // walls behind it. Emissive, so it glows rather than being lit like solid
    // geometry - the same treatment the searchlights get on the menu.
    renderer.setDepthWrite(false);
    renderer.drawPyramid(
        visionConeTransform(apex, guardFacingDeg_, range, width, cfg::kF3ConeHeight),
        playerSeen_ ? cfg::kF3ConeAlert : cfg::kF3ConeCalm,
        playerSeen_ ? cfg::kF3ConeAlpha * 1.8f : cfg::kF3ConeAlpha,
        1.0f);
    renderer.setDepthWrite(true);
}

void StealthScene::drawItems() {
    Renderer& renderer = ctx().renderer;

    const float k = cfg::kF3ItemScale;

    for (int i = 0; i < cfg::kF3ItemCount; ++i) {
        if (itemCollected_[i]) {
            continue;
        }

        const glm::vec3& item = cfg::kF3ItemPositions[i];

        // Offset each item's animation so a row of them does not pulse in
        // lockstep, which reads as a repeating texture rather than as objects.
        const float phase = static_cast<float>(i) * 0.7f;

        const float bob = std::sin(elapsedTime_ * cfg::kF3ItemBobSpeed + phase)
                        * cfg::kF3ItemBobAmount;
        const float spinDeg = elapsedTime_ * cfg::kF3ItemSpinSpeed
                            + phase * 57.0f;

        const glm::vec3 center(item.x, cfg::kF3ItemHoverY + bob, item.z);
        const glm::vec3 spin(0.0f, spinDeg, 0.0f);

        // A faint halo on the deck underneath, to catch the eye from across the
        // room. Kept weak and wide: at full strength it reads as a gold puddle
        // and swallows the paw sitting on top of it.
        renderer.setDepthWrite(false);
        renderer.drawCylinderAt(glm::vec3(item.x, 0.02f, item.z),
                                glm::vec3(1.70f, 0.02f, 1.70f),
                                cfg::kF3ItemGlow, 0.10f, 1.0f);
        renderer.setDepthWrite(true);

        // The pad: a squashed sphere lying flat.
        renderer.drawSphere(
            makeTransform(center, glm::vec3(0.52f, 0.17f, 0.46f) * k, spin),
            cfg::kF3ItemPaw, 1.0f, 0.35f);

        // Four toes in an arc in front of the pad. Rotating each toe's offset
        // by the same spin angle keeps the paw whole as it turns.
        const float spinRad = glm::radians(spinDeg);
        const float sinS = std::sin(spinRad);
        const float cosS = std::cos(spinRad);

        const float toeAngles[4] = {-42.0f, -15.0f, 15.0f, 42.0f};
        for (const float toeAngle : toeAngles) {
            const float a = glm::radians(toeAngle);

            // Local space: +Z is the front of the paw.
            const float lx = std::sin(a) * 0.34f * k;
            const float lz = std::cos(a) * 0.34f * k;

            const glm::vec3 toe(center.x + (lx * cosS + lz * sinS),
                                center.y,
                                center.z + (-lx * sinS + lz * cosS));

            renderer.drawSphere(
                makeTransform(toe, glm::vec3(0.17f, 0.12f, 0.17f) * k, spin),
                cfg::kF3ItemPaw, 1.0f, 0.35f);
        }
    }
}

void StealthScene::drawRoary() {
    Renderer& renderer = ctx().renderer;

    const bool walking = (moveDir_ != glm::vec2(0.0f));

    // The walk bounce, and a slight lean into the direction of travel. Both
    // stop when he does.
    const float bob = walking
        ? std::abs(std::sin(walkCycle_)) * cfg::kF3BobAmount
        : 0.0f;

    const glm::vec3 feet = playerPos_ + glm::vec3(0.0f, bob, 0.0f);

    // --- blob shadow ---------------------------------------------------------
    // Drawn first and flat on the deck. Translucent, so depth writes are off
    // or it would punch a hole in the floor behind it.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(playerPos_.x, 0.015f, playerPos_.z),
                            glm::vec3(cfg::kF3PlayerRadius * 2.3f, 0.02f,
                                      cfg::kF3PlayerRadius * 2.3f),
                            cfg::kF3ShadowColor, cfg::kF3ShadowAlpha, 1.0f);
    renderer.setDepthWrite(true);

    // --- the panther ---------------------------------------------------------
    // Built the same way as the Roary on the level-select screen: squashed
    // spheres at a natural height of 1.42 units, scaled by k. The difference
    // here is that he turns, so every part is positioned by rotating its local
    // offset about Y and is drawn with that same rotation.
    const float k = cfg::kF3PlayerScale;
    const float facingRad = glm::radians(facingDeg_);
    const float sinF = std::sin(facingRad);
    const float cosF = std::cos(facingRad);

    // Local space: +Z is the direction he faces, +X is his left.
    const auto at = [&](float x, float y, float z) {
        return feet + glm::vec3((x * cosF + z * sinF) * k,
                                 y * k,
                                (-x * sinF + z * cosF) * k);
    };
    const auto size = [&](float x, float y, float z) {
        return glm::vec3(x * k, y * k, z * k);
    };
    const glm::vec3 spin(0.0f, facingDeg_, 0.0f);

    const auto sphere = [&](const glm::vec3& center, const glm::vec3& sz,
                            const glm::vec3& color) {
        renderer.drawSphere(makeTransform(center, sz, spin), color);
    };

    const glm::vec3 fur   = cfg::kPantherFur;
    const glm::vec3 cream = cfg::kPantherCream;
    const glm::vec3 kit   = cfg::kFiuKit;

    // Legs swing in opposite phase so the walk reads from above.
    const float stride = walking ? std::sin(walkCycle_) * 0.16f : 0.0f;
    for (int side = 0; side < 2; ++side) {
        const float x    = (side == 0 ? -0.13f : 0.13f);
        const float step = (side == 0 ? stride : -stride);
        renderer.drawCylinder(
            makeTransform(at(x, 0.14f, step), size(0.15f, 0.28f, 0.15f), spin), fur);
    }

    // Navy shorts, then the jersey over a round chest with white lettering.
    sphere(at(0.0f, 0.38f, 0.0f), size(0.50f, 0.28f, 0.44f), kit);
    sphere(at(0.0f, 0.66f, 0.0f), size(0.54f, 0.50f, 0.46f), kit);
    sphere(at(0.0f, 0.70f, 0.21f), size(0.24f, 0.11f, 0.05f), cfg::kFiuKitTrim);

    // Arms, swinging opposite their leg on the same side.
    for (int side = 0; side < 2; ++side) {
        const float x    = (side == 0 ? -0.31f : 0.31f);
        const float step = (side == 0 ? -stride : stride);
        sphere(at(x, 0.64f, step * 0.8f), size(0.15f, 0.34f, 0.15f), fur);
    }

    // Head, muzzle, nose.
    sphere(at(0.0f, 1.06f, 0.0f),  size(0.56f, 0.52f, 0.50f), fur);
    sphere(at(0.0f, 0.97f, 0.19f), size(0.32f, 0.22f, 0.22f), cream);
    sphere(at(0.0f, 1.03f, 0.30f), size(0.09f, 0.07f, 0.06f), cfg::kPantherNose);

    // Ears. Worth the four draws: from this overhead camera they are the part
    // of him that most clearly shows which way he is pointing.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.21f : 0.21f);
        sphere(at(x, 1.30f, 0.00f), size(0.19f, 0.21f, 0.14f), fur);
        sphere(at(x, 1.30f, 0.05f), size(0.10f, 0.12f, 0.08f), cfg::kPantherInner);
    }

    // Eyes.
    for (int side = 0; side < 2; ++side) {
        const float x = (side == 0 ? -0.115f : 0.115f);
        sphere(at(x, 1.12f, 0.20f), size(0.13f, 0.14f, 0.08f), cfg::kEyeWhite);
        sphere(at(x, 1.12f, 0.24f), size(0.07f, 0.08f, 0.05f), cfg::kEyeDark);
    }
}

float StealthScene::px(float designPixels) const {
    return designPixels * uiScale_;
}

void StealthScene::drawUi() {
    TextRenderer& text = ctx().renderer.text();
    const int width  = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    text.begin(width, height);

    const float centerX = static_cast<float>(width) * 0.5f;
    const float bottom  = static_cast<float>(height);
    const cfg::FloorInfo& info = cfg::kFloorInfo[kFloorIndex];

    text.drawCentered(info.name, centerX, px(32.0f),
                      px(cfg::kFloorNameScale), cfg::kTextPrimary);

    text.drawCentered(info.description, centerX, px(80.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    // Paw counter, top right. Laid out the same way the level-select screen
    // puts its "FLOORS CLEARED" counter, so the two screens match.
    const std::string pawLabel = "PAWS " + std::to_string(collectedCount())
                               + "/" + std::to_string(cfg::kF3ItemCount);
    const bool allFound = (collectedCount() == cfg::kF3ItemCount);

    text.draw(pawLabel,
              static_cast<float>(width) - px(24.0f)
                  - TextRenderer::measureWidth(pawLabel, px(cfg::kHintScale)),
              px(24.0f), px(cfg::kHintScale),
              allFound ? cfg::kTextComplete : cfg::kF3ItemPaw);

    // Being seen has no consequence yet - that is the suspicion meter and the
    // fail state, next step. For now it just says so plainly.
    if (playerSeen_) {
        text.drawCentered("SPOTTED", centerX, bottom - px(160.0f),
                          px(cfg::kFloorNameScale), cfg::kF3ConeAlert);
    }

    // Shows the sneak state is actually reaching the simulation. This becomes
    // a real noise indicator once guards can hear, in step 6.
    text.drawCentered(sneaking_ ? "SNEAKING" : "WALKING",
                      centerX, bottom - px(126.0f),
                      px(cfg::kHintScale),
                      sneaking_ ? cfg::kF3Light : cfg::kTextDim);

    text.drawCentered("WASD - MOVE     SHIFT - SNEAK",
                      centerX, bottom - px(90.0f),
                      px(cfg::kHintScale), cfg::kTextPrimary);

    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
