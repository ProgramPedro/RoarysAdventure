//=============================================================================
// StealthScene.cpp - FLOOR 3, "SILENT PAWS"
//
// READ THIS FIRST. This is the biggest file in the project, so here is the map
// of it before you dive in.
//
//-----------------------------------------------------------------------------
// HOW A FRAME WORKS
//
// main.cpp calls three of our methods every frame, always in this order:
//
//   1. handleInput(input)  read the keyboard and mouse. Record WHAT the player
//                          wants. Do not move anything, do not draw anything.
//   2. update(deltaTime)   advance the simulation by deltaTime seconds: move
//                          Roary, walk the guard, fly the rock, fill the
//                          suspicion meter. Still no drawing.
//   3. render()            draw the picture. Read state, never change it.
//
// Keeping those three jobs separate is the single most useful habit in this
// file. When something is in the wrong place it shows up as a bug that is
// hard to describe - "it only happens when I hold two keys", "the meter is
// always one frame behind". Both of those are ordering bugs.
//
// `deltaTime` is how many SECONDS the last frame took. Every speed in this
// file is written in units-per-second and multiplied by deltaTime, so the game
// runs at the same speed on a fast machine and a slow one. If you ever write
// `position += speed;` without the deltaTime, the game gets faster on better
// hardware. main.cpp also clamps deltaTime to 0.1s, so dragging the window
// does not teleport everybody.
//
//-----------------------------------------------------------------------------
// THE WORLD, AND WHICH WAY IS WHICH
//
// The room lies flat on the XZ plane with +Y pointing up at the ceiling:
//
//        -Z  (far wall, away from the camera)
//         |
//   -X ---+--- +X   (right wall, where the exit door is)
//         |
//        +Z  (near wall, the low lip you see over)
//
// Nobody ever jumps, so every character's Y stays 0 and all the interesting
// maths is 2D. That is why you will see so many functions that take a vec3 but
// only ever touch .x and .z.
//
// FACING ANGLES are degrees of rotation about the Y axis, and 0 means looking
// down +Z (toward the camera). They come from atan2(x, z) - note the order,
// x FIRST - which is unusual if you have seen atan2(y, x) in a maths class.
// It is that way because our "forward" is +Z, not +X. Roary, the guard, the
// thrown rock and the vision cone all share this convention; if you add a new
// character, follow it or things will point sideways.
//
//-----------------------------------------------------------------------------
// WHAT IS IN THIS FILE, IN ORDER
//
//   file-local helpers     angle maths, the aspect-ratio fix, the cone matrix
//   init / returnToMenu    setting the level up
//   Input                  reading keys, aiming and throwing
//   Simulation             collision, pickups, the door
//   Navigation             the walkable grid and DIJKSTRA pathfinding
//   Vision                 what the guard can see, and line of sight
//   Movement and update    moving Roary, and the per-frame update order
//   Rocks                  mouse picking, the throw, the noise it makes
//   The guard              patrol, distraction, suspicion, getting caught
//   Drawing                every draw* function, plus the HUD
//
// Anything you might want to TUNE - sizes, speeds, colours, the room layout,
// the patrol route - is not in this file at all. It is in core/Config.h, in
// the sections beginning kF3. Change numbers there, not here.
//=============================================================================

#include "scenes/StealthScene.h"

#include "scenes/Characters.h"   // drawRoary() and shortestAngleDelta()

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
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

// Standard library. <queue> and <limits> are here for the pathfinder: it needs
// a priority queue to always pull out the cheapest cell next, and "infinity"
// as the starting cost for every cell it has not reached yet.
#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <cmath>
#include <string>
#include <utility>

namespace {



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

//-----------------------------------------------------------------------------
// A wedge whose APEX sits at `apex` and whose body runs `range` units along
// `facingDeg`. This is the guard's vision cone.
//
// MATRICES READ RIGHT TO LEFT, which is the thing that trips everyone up at
// first. The last line below (scale) happens to the shape FIRST, and the first
// line (translate) happens LAST. So read this function bottom-up to follow
// what actually happens to the pyramid:
//
//   4. scale      stretch the unit pyramid to the cone's length and width
//   3. translate  shift it back along its own axis so its TIP is at the
//                 origin - without this it would grow around its middle and
//                 half the cone would come out of the back of the guard's head
//   2. rotate X   tip it over, so the pyramid (which points up +Y by default)
//                 instead lies flat pointing along +Z
//   1. rotate Y   swing it round to the direction the guard is facing
//   0. translate  finally move the whole thing to the guard
//
// Same idea as hangingTransform() in LevelSelectScene, one axis over.
//-----------------------------------------------------------------------------
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
    // 180 degrees looks down -Z: away from the camera and into the room, which
    // is the direction he has to travel. (0 would face back at the camera.)
    facingDeg_   = 180.0f;
    walkCycle_   = 0.0f;
    sneaking_    = false;
    elapsedTime_ = 0.0f;

    guardPos_            = cfg::kF3GuardStart;
    guardFacingDeg_      = cfg::kF3GuardFacing;
    guardHomeFacingDeg_  = cfg::kF3GuardFacing;
    guardAlertTimer_     = 0.0f;
    guardPauseTimer_     = 0.0f;
    patrolIndex_         = 0;

    suspicion_        = 0.0f;
    sightLostTimer_   = 0.0f;
    caughtFlashTimer_ = 0.0f;

    // The grid never changes, so it is built once here rather than per path.
    buildNavGrid();
    repathGuard();

    itemCollected_.fill(false);
    playerSeen_       = false;
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

    // --- rocks ---------------------------------------------------------------
    // E toggles aiming. wasPressed(), not isDown(): holding the key should not
    // flicker the mode on and off every frame.
    if (input.wasPressed(GLFW_KEY_E)) {
        if (aiming_) {
            aiming_ = false;              // pressing it again cancels
        } else if (rocksLeft_ > 0 && !rockInFlight_) {
            aiming_ = true;
        }
    }

    // Cancel aiming the moment the last rock is gone or one is already up.
    if (aiming_ && (rocksLeft_ <= 0 || rockInFlight_)) {
        aiming_ = false;
    }

    if (aiming_) {
        updateAim(input);

        if (input.wasMouseClicked()) {
            throwRock();
        }
    }
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

bool StealthScene::doorIsOpen() const {
    return collectedCount() == cfg::kF3ItemCount;
}

void StealthScene::finishLevel() {
    // The real win condition at last. markCompleted() marks floor 3 cleared
    // AND unlocks the floor above it; the level-select screen reads
    // GameProgress fresh every frame, so the change shows immediately.
    ctx().progress.markCompleted(kFloorIndex);
    returnToMenu();
}

void StealthScene::checkDoor() {
    // The point on the inner face of the right wall that the door sits in.
    const float dx = cfg::kF3RoomHalfX - playerPos_.x;
    const float dz = cfg::kF3DoorZ     - playerPos_.z;

    if (dx * dx + dz * dz > cfg::kF3DoorTriggerRadius * cfg::kF3DoorTriggerRadius) {
        return;
    }

    if (doorIsOpen()) {
        finishLevel();
        return;
    }

    // Locked. Restart the flash rather than letting it tick down while he is
    // still standing there, so the message stays up for as long as he tries.
    lockedFlashTimer_ = cfg::kF3LockedFlashTime;
}

// How many paws Roary is carrying. Counted on demand rather than kept in a
// variable: with four items it costs nothing, and one less piece of state is
// one less thing that can get out of step with the truth.
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
        if (itemCollected_[static_cast<std::size_t>(i)]) {
            continue;
        }

        const glm::vec3& item = cfg::kF3ItemPositions[i];
        const float dx = item.x - playerPos_.x;
        const float dz = item.z - playerPos_.z;

        // Distance on the floor only. Roary's Y never changes, and the item
        // hovers, so including Y would make the hover height part of the
        // pickup range by accident.
        if (dx * dx + dz * dz <= radiusSquared) {
            itemCollected_[static_cast<std::size_t>(i)] = true;
        }
    }
}

//=============================================================================
// Navigation: a grid of the walkable deck, and Dijkstra over it
//=============================================================================

void StealthScene::buildNavGrid() {
    navCountX_ = static_cast<int>(cfg::kF3RoomHalfX * 2.0f / cfg::kF3NavCell);
    navCountZ_ = static_cast<int>(cfg::kF3RoomHalfZ * 2.0f / cfg::kF3NavCell);

    navOpen_.assign(static_cast<std::size_t>(navCountX_ * navCountZ_), 0);

    for (int ix = 0; ix < navCountX_; ++ix) {
        for (int iz = 0; iz < navCountZ_; ++iz) {
            const glm::vec3 centre = navCellCenter(ix, iz);
            const float radius = cfg::kF3GuardRadius;

            // Inside the outer walls with room for his whole body...
            const bool insideRoom =
                std::fabs(centre.x) <= cfg::kF3RoomHalfX - radius &&
                std::fabs(centre.z) <= cfg::kF3RoomHalfZ - radius;

            // ...and clear of every interior wall. Testing the body rather
            // than the centre point is what stops a path being plotted
            // through a gap the guard cannot actually fit through.
            bool clear = insideRoom;

            for (int i = 0; i < cfg::kF3InnerWallCount && clear; ++i) {
                const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];

                if (centre.x + radius > wall.centerX - wall.sizeX * 0.5f &&
                    centre.x - radius < wall.centerX + wall.sizeX * 0.5f &&
                    centre.z + radius > wall.centerZ - wall.sizeZ * 0.5f &&
                    centre.z - radius < wall.centerZ + wall.sizeZ * 0.5f) {
                    clear = false;
                }
            }

            navOpen_[static_cast<std::size_t>(ix * navCountZ_ + iz)] =
                clear ? 1 : 0;
        }
    }
}

// Can the guard stand in this cell? Also doubles as the bounds check: a cell
// index off the edge of the grid simply answers "no", which saves every caller
// below from testing the edges separately.
bool StealthScene::navCellOpen(int ix, int iz) const {
    if (ix < 0 || iz < 0 || ix >= navCountX_ || iz >= navCountZ_) {
        return false;
    }
    return navOpen_[static_cast<std::size_t>(ix * navCountZ_ + iz)] != 0;
}

// Grid square -> world position. Cell (0,0) is the far-left corner of the
// room, and the + 0.5f puts us in the MIDDLE of the square rather than on its
// corner, which is where we actually want the guard to walk.
glm::vec3 StealthScene::navCellCenter(int ix, int iz) const {
    return glm::vec3(
        -cfg::kF3RoomHalfX + (static_cast<float>(ix) + 0.5f) * cfg::kF3NavCell,
        0.0f,
        -cfg::kF3RoomHalfZ + (static_cast<float>(iz) + 0.5f) * cfg::kF3NavCell);
}

// World position -> grid square. The reverse of navCellCenter().
//
// If the square we land on is blocked (because somebody wrote a patrol point
// slightly inside a wall in Config.h), we do not give up - we search for the
// nearest open square instead. That linear scan looks wasteful, but this runs
// about once every few seconds when the guard picks a new target, not per
// frame, so clarity beats cleverness here.
bool StealthScene::navCellFor(const glm::vec3& position,
                              int& outX, int& outZ) const {
    outX = static_cast<int>((position.x + cfg::kF3RoomHalfX) / cfg::kF3NavCell);
    outZ = static_cast<int>((position.z + cfg::kF3RoomHalfZ) / cfg::kF3NavCell);

    if (navCellOpen(outX, outZ)) {
        return true;
    }

    // The exact cell is blocked - which happens whenever a patrol point is
    // written a little too close to a wall. Rather than failing, take the
    // nearest open cell, so a slightly sloppy Config entry still works.
    float bestDistance = std::numeric_limits<float>::max();
    int bestX = -1;
    int bestZ = -1;

    for (int ix = 0; ix < navCountX_; ++ix) {
        for (int iz = 0; iz < navCountZ_; ++iz) {
            if (!navCellOpen(ix, iz)) {
                continue;
            }

            const glm::vec3 centre = navCellCenter(ix, iz);
            const float dx = centre.x - position.x;
            const float dz = centre.z - position.z;
            const float distance = dx * dx + dz * dz;

            if (distance < bestDistance) {
                bestDistance = distance;
                bestX = ix;
                bestZ = iz;
            }
        }
    }

    if (bestX < 0) {
        return false;
    }

    outX = bestX;
    outZ = bestZ;
    return true;
}

bool StealthScene::sweepIsClear(const glm::vec3& from, const glm::vec3& to,
                                float radius) const {
    // Walk the segment in steps smaller than a cell and check the body fits
    // at each one. Cruder than a swept box, but it reuses the same occupancy
    // test the grid was built from, so the smoothing can never disagree with
    // the search about what is walkable.
    const float dx = to.x - from.x;
    const float dz = to.z - from.z;
    const float length = std::sqrt(dx * dx + dz * dz);

    const int steps = std::max(2, static_cast<int>(length / (cfg::kF3NavCell * 0.5f)));

    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const glm::vec3 point = from + glm::vec3(dx, 0.0f, dz) * t;

        if (std::fabs(point.x) > cfg::kF3RoomHalfX - radius ||
            std::fabs(point.z) > cfg::kF3RoomHalfZ - radius) {
            return false;
        }

        for (int w = 0; w < cfg::kF3InnerWallCount; ++w) {
            const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[w];

            if (point.x + radius > wall.centerX - wall.sizeX * 0.5f &&
                point.x - radius < wall.centerX + wall.sizeX * 0.5f &&
                point.z + radius > wall.centerZ - wall.sizeZ * 0.5f &&
                point.z - radius < wall.centerZ + wall.sizeZ * 0.5f) {
                return false;
            }
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// DIJKSTRA'S ALGORITHM - the shortest walk from one point to another
//
// If you have not met it before, the idea is simpler than the name:
//
//   1. Give every cell a "cost so far", starting at infinity. The cell we
//      start on costs 0.
//   2. Repeatedly take the UNVISITED CELL WITH THE SMALLEST COST and look at
//      its neighbours. If we can reach a neighbour more cheaply than its
//      current best, write down the cheaper cost and remember which cell we
//      came from.
//   3. Stop when the cell we pull out is the goal.
//   4. Follow the "came from" trail backwards from the goal to get the route.
//
// Step 2 is why we need a priority_queue: it hands back the smallest item
// first, so we always expand the most promising cell next. Because we always
// expand the cheapest cell, the first time we reach the goal we are guaranteed
// to have reached it by the cheapest route - that guarantee is the whole point
// of the algorithm.
//
// Cost here is real distance: 1 for a step up/down/left/right, and sqrt(2)
// (about 1.414) for a diagonal, because the diagonal of a square really is
// that much longer than its side. If you used 1 for diagonals too, the search
// would happily produce zig-zag routes that look wrong.
//
// Returns false if there is genuinely no way through.
//-----------------------------------------------------------------------------
bool StealthScene::findPath(const glm::vec3& from, const glm::vec3& to,
                            std::vector<glm::vec3>& outPath) const {
    outPath.clear();

    int startX = 0, startZ = 0, goalX = 0, goalZ = 0;
    if (!navCellFor(from, startX, startZ) || !navCellFor(to, goalX, goalZ)) {
        return false;
    }

    const int cellCount = navCountX_ * navCountZ_;
    const int startId = startX * navCountZ_ + startZ;
    const int goalId  = goalX  * navCountZ_ + goalZ;

    if (startId == goalId) {
        outPath.push_back(to);
        return true;
    }

    // --- Dijkstra ------------------------------------------------------------
    // A plain uniform-cost search. A* with a straight-line heuristic would
    // visit fewer cells, but on a grid this size the whole search is a
    // fraction of a millisecond and only runs when he picks a new target, so
    // the simpler algorithm is the better trade.
    const float kInfinity = std::numeric_limits<float>::max();

    std::vector<float> distance(static_cast<std::size_t>(cellCount), kInfinity);
    std::vector<int>   cameFrom(static_cast<std::size_t>(cellCount), -1);

    using Entry = std::pair<float, int>;   // (cost so far, cell id)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> frontier;

    distance[static_cast<std::size_t>(startId)] = 0.0f;
    frontier.emplace(0.0f, startId);

    const int   stepX[8] = { 1, -1,  0,  0,  1,  1, -1, -1 };
    const int   stepZ[8] = { 0,  0,  1, -1,  1, -1,  1, -1 };
    const float stepCost[8] = { 1.0f, 1.0f, 1.0f, 1.0f,
                                1.41421356f, 1.41421356f,
                                1.41421356f, 1.41421356f };

    bool reached = false;

    while (!frontier.empty()) {
        const Entry current = frontier.top();
        frontier.pop();

        const int id = current.second;

        // Stale queue entry: we already found a cheaper way in. Skipping
        // these is what lets us push duplicates instead of needing a
        // decrease-key operation.
        if (current.first > distance[static_cast<std::size_t>(id)]) {
            continue;
        }

        if (id == goalId) {
            reached = true;
            break;
        }

        const int ix = id / navCountZ_;
        const int iz = id % navCountZ_;

        for (int d = 0; d < 8; ++d) {
            const int nx = ix + stepX[d];
            const int nz = iz + stepZ[d];

            if (!navCellOpen(nx, nz)) {
                continue;
            }

            // A diagonal may not cut a corner: both neighbouring orthogonal
            // cells have to be open too, or he clips the end of a wall while
            // slipping between two blocked cells.
            if (stepX[d] != 0 && stepZ[d] != 0) {
                if (!navCellOpen(ix + stepX[d], iz) ||
                    !navCellOpen(ix, iz + stepZ[d])) {
                    continue;
                }
            }

            const int neighbourId = nx * navCountZ_ + nz;
            const float candidate =
                distance[static_cast<std::size_t>(id)] + stepCost[d];

            if (candidate < distance[static_cast<std::size_t>(neighbourId)]) {
                distance[static_cast<std::size_t>(neighbourId)] = candidate;
                cameFrom[static_cast<std::size_t>(neighbourId)] = id;
                frontier.emplace(candidate, neighbourId);
            }
        }
    }

    if (!reached) {
        return false;
    }

    // --- walk the parents back, then straighten ------------------------------
    std::vector<glm::vec3> reversed;
    for (int id = goalId; id != -1; id = cameFrom[static_cast<std::size_t>(id)]) {
        reversed.push_back(navCellCenter(id / navCountZ_, id % navCountZ_));
        if (id == startId) {
            break;
        }
    }

    std::reverse(reversed.begin(), reversed.end());

    // String-pulling: from each kept point, skip ahead to the furthest point
    // still reachable in a straight line. Without this the guard visibly
    // jinks from cell centre to cell centre across open floor.
    const float radius = cfg::kF3GuardRadius;
    std::size_t index = 0;

    while (index < reversed.size()) {
        std::size_t furthest = index + 1;

        for (std::size_t probe = index + 1; probe < reversed.size(); ++probe) {
            if (sweepIsClear(reversed[index], reversed[probe], radius)) {
                furthest = probe;
            }
        }

        if (furthest >= reversed.size()) {
            break;
        }

        outPath.push_back(reversed[furthest]);
        index = furthest;
    }

    // Finish on the real target rather than the centre of its cell, so he
    // stands where Config asked rather than up to half a cell away.
    if (sweepIsClear(outPath.empty() ? from : outPath.back(), to, radius)) {
        outPath.push_back(to);
    }

    return !outPath.empty();
}

// Work out a fresh route to whichever patrol point the guard is heading for.
// Called whenever his situation changes: at startup, when he finishes a point,
// and when he stops being distracted (because he is no longer standing where
// his old path began).
void StealthScene::repathGuard() {
    guardPathNode_ = 0;
    guardPath_.clear();

    if (cfg::kF3PatrolCount <= 0) {
        return;
    }

    const glm::vec3& target = cfg::kF3PatrolPoints[patrolIndex_];

    if (!findPath(guardPos_, target, guardPath_)) {
        // Unreachable. Skip it rather than freezing: a patrol point walled off
        // by an edit to kF3InnerWalls should cost one stop, not the guard.
        patrolIndex_ = (patrolIndex_ + 1) % cfg::kF3PatrolCount;
    }
}

bool StealthScene::firstWallHit(const glm::vec3& from, const glm::vec3& to,
                                float& outT) {
    //-------------------------------------------------------------------------
    // THE SLAB TEST - does a line segment hit an axis-aligned box?
    //
    // Think of the segment as a journey where t = 0 is the start and t = 1 is
    // the end, so any point along it is  from + (to - from) * t.
    //
    // A box is the overlap of two "slabs": the vertical strip between its left
    // and right edges, and the horizontal strip between its front and back
    // edges. For each slab we work out the range of t during which the journey
    // is INSIDE that strip. If those two ranges overlap, there is a moment
    // when we are inside both strips at once - and being inside both strips at
    // once is exactly what being inside the box means.
    //
    //        enter                exit
    //          |                   |
    //   -------[===================]--------   t inside the X strip
    //      ---------[==========]------------   t inside the Z strip
    //               ^          ^
    //               overlap: the segment really does cross the box
    //
    // So we keep `enter` (the latest any slab was entered) and `exit` (the
    // earliest any slab was left). If enter ever passes exit, the ranges do
    // not overlap and this box is missed.
    //
    // Unlike a pure yes/no test we keep `enter` itself, because a thrown rock
    // has to STOP at the first wall, not merely know that one is in the way.
    //-------------------------------------------------------------------------
    const float dx = to.x - from.x;
    const float dz = to.z - from.z;

    float nearest = 1.0f;
    bool  hitAny  = false;

    for (int i = 0; i < cfg::kF3InnerWallCount; ++i) {
        const cfg::F3WallSlab& wall = cfg::kF3InnerWalls[i];

        const float origin[2] = {from.x, from.z};
        const float delta[2]  = {dx, dz};
        const float lower[2]  = {wall.centerX - wall.sizeX * 0.5f,
                                 wall.centerZ - wall.sizeZ * 0.5f};
        const float upper[2]  = {wall.centerX + wall.sizeX * 0.5f,
                                 wall.centerZ + wall.sizeZ * 0.5f};

        float enter = 0.0f;
        float exit  = 1.0f;
        bool  missed = false;

        // One pass per axis. Written as a loop over {x, z} so the near-zero
        // case - a segment running exactly parallel to a wall - is handled the
        // same way on both axes instead of being special-cased twice.
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

        if (!missed && enter < nearest) {
            nearest = enter;
            hitAny  = true;
        }
    }

    outT = nearest;
    return hitAny;
}

// "Is there a wall between these two points?" - the line-of-sight test. This
// is just firstWallHit() with the distance thrown away, kept as its own name
// because that is how the vision code wants to read.
bool StealthScene::lineCrossesWall(const glm::vec3& from, const glm::vec3& to) {
    float ignored = 0.0f;
    return firstWallHit(from, to, ignored);
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

// Moves Roary by whatever handleInput() recorded this frame, then pushes him
// back out of anything he walked into.
//
// The ORDER here is the important part - see the comment on the axis handling
// below for why moving X and Z separately is what gives you wall sliding.
void StealthScene::movePlayer(float deltaTime) {
    const bool walking = (moveDir_ != glm::vec2(0.0f));

    if (walking) {
        const float speed = sneaking_ ? cfg::kF3SneakSpeed : cfg::kF3WalkSpeed;
        const float radius = cfg::kF3PlayerRadius;

        const float deltaX = moveDir_.x * speed * deltaTime;
        const float deltaZ = moveDir_.y * speed * deltaTime;

        // ONE AXIS AT A TIME, each resolved before the next moves. This is the
        // trick that gives you wall sliding, and it is worth seeing why.
        //
        // Say Roary walks diagonally into a wall that runs north-south:
        //
        //   Move X  -> he is now inside the wall -> push him back out on X.
        //              His X is cancelled, but his Z has not been touched.
        //   Move Z  -> nothing is in the way, so this one goes through.
        //
        // Net result: he slides ALONG the wall. If instead you moved both axes
        // first and then resolved, you would have to guess which direction to
        // push him out of, and the usual answer (the shallowest one) makes him
        // stick on corners and jitter.
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
    //-------------------------------------------------------------------------
    // THE ORDER OF THESE CALLS IS THE DESIGN, NOT AN ACCIDENT.
    //
    // Every line below depends on the ones above it having already happened
    // this frame. Reordering them does not crash anything - it produces
    // subtle, annoying bugs like a meter that lags a frame behind the guard,
    // or a paw that does not count until the frame after you touch the door.
    // Each step says what it needs from the step before.
    //-------------------------------------------------------------------------
    elapsedTime_ += deltaTime;

    // 1. Move Roary first. Everything after this wants his FINAL position for
    //    this frame, not the one he started on.
    movePlayer(deltaTime);

    // 2. Pick up anything he is now standing on.
    collectNearbyItems();

    // 3. A first look at whether he is visible. The guard has not moved yet,
    //    so this is "can the guard see him from where the guard was".
    playerSeen_ = guardCanSeePlayer();

    // 4. The door. After the pickups, so a paw collected on this very frame
    //    already counts toward opening it - otherwise grabbing the last paw
    //    while touching the door would flash LOCKED for one frame.
    checkDoor();

    // 5. Fly the rock, and raise the noise if it lands this frame.
    updateRock(deltaTime);

    // 6. Walk the guard. After updateRock, so a rock that lands this frame
    //    turns his head this frame rather than on the next one.
    updateGuard(deltaTime);

    // 7. The guard has now moved, so what he can see has CHANGED. Check again
    //    before the meter reads it, or the meter is always one frame behind
    //    him - which the player experiences as being caught by a guard who
    //    appears not to be looking at them.
    playerSeen_ = guardCanSeePlayer();
    updateSuspicion(deltaTime);

    // 8. Finally, tick down the simple countdown timers.
    lockedFlashTimer_ = std::max(0.0f, lockedFlashTimer_ - deltaTime);
}

//=============================================================================
// Drawing
//=============================================================================

glm::mat4 StealthScene::cameraViewProjection() const {
    const int width  = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    const float aspect = static_cast<float>(width)
                       / static_cast<float>(std::max(height, 1));

    Camera camera(cfg::kF3CameraPos, cfg::kF3CameraTarget);
    camera.setFovDegrees(fovForAspect(cfg::kF3CameraFovDeg, aspect));

    return camera.perspective(width, height) * camera.view();
}

bool StealthScene::screenToFloor(const glm::vec2& screenPos,
                                 glm::vec3& outFloor) const {
    const float width  = static_cast<float>(ctx().framebufferWidth);
    const float height = static_cast<float>(ctx().framebufferHeight);

    //-------------------------------------------------------------------------
    // TURNING A MOUSE POSITION INTO A POINT ON THE FLOOR
    //
    // Drawing goes  world point -> matrix -> pixel on screen.  Here we need to
    // run that backwards, and the catch is that it does not invert cleanly: a
    // single pixel does not correspond to one point in the world, it
    // corresponds to a whole RAY shooting away from the camera through that
    // pixel. Everything along that ray lands on the same pixel.
    //
    // So we do it in three steps:
    //   1. pixels -> normalized device coordinates (NDC), the -1..1 box that
    //      OpenGL works in
    //   2. push two NDC points (one near, one far) through the INVERSE of the
    //      camera matrix to get two world points - together they define the ray
    //   3. find where that ray crosses the floor plane y = 0
    //
    // Step 3 is the only part that is specific to this game, and it is what
    // makes the answer unique: of all the points along the ray, we want the
    // one lying on the deck.
    //-------------------------------------------------------------------------

    // Step 1. Pixels -> NDC. The y flip is because the cursor's origin is the
    // top-left with y growing down, while NDC's origin is the centre with y
    // growing up.
    const float ndcX = (screenPos.x / width) * 2.0f - 1.0f;
    const float ndcY = 1.0f - (screenPos.y / height) * 2.0f;

    const glm::mat4 inverseVp = glm::inverse(cameraViewProjection());

    // Step 2. Two points on the cursor's ray: z = -1 is the near plane and
    // z = +1 is the far plane, which is how OpenGL's depth range is laid out.
    const glm::vec4 nearPoint = inverseVp * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    const glm::vec4 farPoint  = inverseVp * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);

    if (std::fabs(nearPoint.w) < 1e-6f || std::fabs(farPoint.w) < 1e-6f) {
        return false;
    }

    // The divide by w is the "perspective divide". A perspective matrix stashes
    // a depth-dependent number in w, and dividing by it is what makes distant
    // things smaller. Undoing the matrix means undoing that divide too -
    // forget it and the point comes back in the wrong place.
    const glm::vec3 rayStart = glm::vec3(nearPoint) / nearPoint.w;
    const glm::vec3 rayEnd   = glm::vec3(farPoint) / farPoint.w;
    const glm::vec3 rayDir   = rayEnd - rayStart;

    // Step 3. Where does the ray cross the deck at y = 0?
    //
    // A point on the ray is  rayStart + rayDir * t.  We want the t where the
    // HEIGHT is zero, so solve:
    //
    //     rayStart.y + rayDir.y * t = 0
    //                             t = -rayStart.y / rayDir.y
    //
    // If rayDir.y is zero the ray runs parallel to the floor and never meets
    // it, and a negative t means the floor is BEHIND the camera - which is
    // what happens when the cursor is above the horizon.
    if (std::fabs(rayDir.y) < 1e-6f) {
        return false;
    }

    const float t = -rayStart.y / rayDir.y;
    if (t < 0.0f) {
        return false;
    }

    outFloor = rayStart + rayDir * t;
    outFloor.y = 0.0f;
    return true;
}

// Runs every frame while the player is holding the aiming mode open. Works out
// two different points:
//
//   aimTarget_  where the cursor is pointing (clamped to the room and to how
//               far Roary can actually throw)
//   aimImpact_  where the rock would really end up, which is the same place
//               UNLESS a wall gets in the way first
//
// The marker is drawn at aimImpact_, so the player is always choosing where
// the noise will happen rather than where they happen to be pointing.
void StealthScene::updateAim(const Input& input) {
    glm::vec3 floorPoint(0.0f);
    aimValid_ = screenToFloor(input.mousePosition(), floorPoint);

    if (!aimValid_) {
        return;
    }

    // Keep the target on the deck. Half a radius in from the wall, so a rock
    // aimed at the edge does not look like it is embedded in it.
    floorPoint = clampToRoom(floorPoint, cfg::kF3RockRadius * 2.0f);

    // And within arm's reach. Pointing past the limit throws as far as he can
    // in that direction rather than refusing, which is far less fiddly.
    const glm::vec3 fromPlayer = floorPoint - playerPos_;
    const float distance = std::sqrt(fromPlayer.x * fromPlayer.x +
                                     fromPlayer.z * fromPlayer.z);

    if (distance > cfg::kF3RockThrowRange && distance > 1e-4f) {
        const float scale = cfg::kF3RockThrowRange / distance;
        floorPoint = playerPos_ + glm::vec3(fromPlayer.x * scale, 0.0f,
                                            fromPlayer.z * scale);
    }

    aimTarget_ = floorPoint;

    // Now the part that matters: does it get there? If a wall is in the way
    // the rock stops at the wall, and the marker shows THAT spot, so the
    // player is choosing where the noise happens rather than where they
    // pointed.
    float hitT = 1.0f;
    aimBlocked_ = firstWallHit(playerPos_, aimTarget_, hitT);

    if (aimBlocked_) {
        // Pull back by the rock's radius so it rests against the wall rather
        // than halfway inside it.
        const glm::vec3 path = aimTarget_ - playerPos_;
        const float pathLength = std::sqrt(path.x * path.x + path.z * path.z);
        const float backOff = (pathLength > 1e-4f)
            ? std::min(hitT, std::max(0.0f, hitT - cfg::kF3RockRadius / pathLength))
            : hitT;

        aimImpact_ = playerPos_ + path * backOff;
    } else {
        aimImpact_ = aimTarget_;
    }

    aimImpact_.y = 0.0f;
}

// Launches a rock at the current aim. Sets up the start, the end and how long
// the flight should take; updateRock() then animates it and raises the noise
// when it arrives.
void StealthScene::throwRock() {
    if (rocksLeft_ <= 0 || rockInFlight_ || !aimValid_) {
        return;
    }

    --rocksLeft_;
    aiming_ = false;

    rockFrom_ = playerPos_;
    rockTo_   = aimImpact_;

    const float dx = rockTo_.x - rockFrom_.x;
    const float dz = rockTo_.z - rockFrom_.z;
    const float distance = std::sqrt(dx * dx + dz * dz);

    // Flight time from the distance, so a short lob is quick and a long one
    // hangs. A floor keeps a rock dropped at his feet from arriving in zero
    // seconds and skipping the animation entirely.
    rockDuration_ = std::max(0.18f, distance / cfg::kF3RockSpeed);
    rockElapsed_  = 0.0f;
    rockInFlight_ = true;

    // Face the throw. Satisfying, and it means the animation reads even when
    // the rock itself is a few pixels across.
    facingDeg_ = glm::degrees(std::atan2(dx, dz));
}

//=============================================================================
// The guard: hearing, patrol, suspicion
//
// Everything the guard does. He has three things on his mind, in order of
// priority: a noise he just heard, the patrol point he is walking to, and
// whether he can see Roary.
//
// Note that he never decides anything in render() - he is fully simulated
// here, and the drawing code only ever reads the result.
//=============================================================================

void StealthScene::raiseNoise(const glm::vec3& position) {
    noisePos_   = position;
    noiseTimer_ = cfg::kF3NoiseDuration;

    // Does the guard hear it? Plain distance, with no line-of-sight test:
    // sound travels around walls, and a rock lobbed through a doorway to pull
    // him off the room he is watching is exactly what rocks are for.
    const float dx = position.x - guardPos_.x;
    const float dz = position.z - guardPos_.z;

    if (dx * dx + dz * dz <= cfg::kF3NoiseRadius * cfg::kF3NoiseRadius) {
        // Remember the heading he was walking on, so that after the four
        // seconds are up he turns back to it rather than to some stale angle
        // from several patrol points ago.
        if (!guardIsDistracted()) {
            guardHomeFacingDeg_ = guardFacingDeg_;
        }

        guardAlertTimer_  = cfg::kF3GuardAlertTime;
        guardAlertTarget_ = position;
    }
}

void StealthScene::updateGuard(float deltaTime) {
    if (guardAlertTimer_ > 0.0f) {
        guardAlertTimer_ = std::max(0.0f, guardAlertTimer_ - deltaTime);

        // The moment he loses interest, work out a fresh route from wherever
        // he stopped. His old path started somewhere he is no longer standing.
        if (guardAlertTimer_ <= 0.0f) {
            repathGuard();
        }
    }

    float targetFacingDeg = guardHomeFacingDeg_;

    if (guardIsDistracted()) {
        // Distracted: he stops dead and looks toward the sound. He does not
        // walk over to investigate - that would be the next behaviour to add,
        // and the pathfinder is already here to do it.
        const float dx = guardAlertTarget_.x - guardPos_.x;
        const float dz = guardAlertTarget_.z - guardPos_.z;

        // A rock landing on top of him gives no direction to turn toward, so
        // he just keeps looking where he was.
        if (std::fabs(dx) > 1e-4f || std::fabs(dz) > 1e-4f) {
            targetFacingDeg = glm::degrees(std::atan2(dx, dz));
        }
    } else if (guardPauseTimer_ > 0.0f) {
        // Standing at a patrol point having a look around.
        guardPauseTimer_ = std::max(0.0f, guardPauseTimer_ - deltaTime);

        if (guardPauseTimer_ <= 0.0f) {
            patrolIndex_ = (patrolIndex_ + 1) % cfg::kF3PatrolCount;
            repathGuard();
        }
    } else if (!guardPath_.empty()) {
        // Walking. Head for the current node; when it is reached, move on to
        // the next, and when the path runs out, pause and take the next
        // patrol point.
        const glm::vec3& node = guardPath_[guardPathNode_];

        const float dx = node.x - guardPos_.x;
        const float dz = node.z - guardPos_.z;
        const float distance = std::sqrt(dx * dx + dz * dz);

        if (distance <= cfg::kF3GuardArriveDist) {
            ++guardPathNode_;

            if (guardPathNode_ >= guardPath_.size()) {
                guardPath_.clear();
                guardPathNode_   = 0;
                guardPauseTimer_ = cfg::kF3GuardPauseTime;

                // Hold this heading while he pauses, instead of snapping back
                // to whatever he was facing several points ago.
                guardHomeFacingDeg_ = guardFacingDeg_;
            }
        } else {
            const float step = std::min(cfg::kF3GuardSpeed * deltaTime, distance);
            guardPos_.x += (dx / distance) * step;
            guardPos_.z += (dz / distance) * step;

            targetFacingDeg = glm::degrees(std::atan2(dx, dz));
            guardHomeFacingDeg_ = targetFacingDeg;
        }
    } else {
        // Nothing to do and nowhere to be: ask for a route again. Covers the
        // first frame, and a path that failed to build.
        repathGuard();
    }

    // Same eased turn Roary uses, including the 0..1 clamp that stops a long
    // frame from overshooting the target and jittering around it.
    const float rate = guardIsDistracted() ? cfg::kF3GuardAlertTurnRate
                                           : cfg::kF3GuardTurnRate;
    const float turn = std::min(rate * deltaTime, 1.0f);
    guardFacingDeg_ += shortestAngleDelta(guardFacingDeg_, targetFacingDeg) * turn;
}

void StealthScene::getCaught() {
    // Back to the start. Paws and rocks are KEPT on purpose: losing them would
    // turn one mistake into replaying the whole level, which punishes the
    // experimenting that stealth is supposed to reward.
    playerPos_ = clampToRoom(cfg::kF3StartPos, cfg::kF3PlayerRadius);
    moveDir_   = glm::vec2(0.0f);
    facingDeg_ = 180.0f;

    suspicion_        = 0.0f;
    sightLostTimer_   = 0.0f;
    playerSeen_       = false;
    caughtFlashTimer_ = cfg::kF3CaughtFlashTime;

    aiming_       = false;
    rockInFlight_ = false;
    noiseTimer_   = 0.0f;

    // The guard goes back on patrol from the top, so the route is predictable
    // on a retry rather than depending on where he happened to be.
    guardPos_            = cfg::kF3GuardStart;
    guardFacingDeg_      = cfg::kF3GuardFacing;
    guardHomeFacingDeg_  = cfg::kF3GuardFacing;
    guardAlertTimer_     = 0.0f;
    guardPauseTimer_     = 0.0f;
    patrolIndex_         = 0;
    repathGuard();
}

// The suspicion meter: fills while the guard has eyes on Roary, drains when he
// does not, and fails the level when it reaches 1.
//
// Why a meter instead of just catching the player the instant they are seen?
// Because instant detection feels unfair and is miserable to playtest. A meter
// gives you a second to react - the "get back behind the wall" moment that is
// the whole pleasure of a stealth game.
void StealthScene::updateSuspicion(float deltaTime) {
    if (caughtFlashTimer_ > 0.0f) {
        caughtFlashTimer_ = std::max(0.0f, caughtFlashTimer_ - deltaTime);
    }

    if (playerSeen_) {
        sightLostTimer_ = cfg::kF3SuspicionGrace;

        // Fills faster the closer he is, so being clocked across the room is a
        // warning and being clocked at his feet is not.
        const float dx = playerPos_.x - guardPos_.x;
        const float dz = playerPos_.z - guardPos_.z;
        const float distance = std::sqrt(dx * dx + dz * dz);

        // `closeness` is 1 when Roary is right on top of the guard and 0 at the
        // very edge of his sight. Dividing by the range turns a distance in
        // world units into a 0..1 fraction, and subtracting from 1 flips it so
        // that bigger means nearer.
        const float closeness =
            1.0f - std::clamp(distance / cfg::kF3VisionRange, 0.0f, 1.0f);

        // Then blend between the two rates from Config using that fraction.
        // This pattern - start + (end - start) * amount - is linear
        // interpolation, or "lerp", and it turns up everywhere in games:
        //
        //     closeness = 0  ->  rate = FarRate       (barely noticed)
        //     closeness = 1  ->  rate = NearRate      (caught quickly)
        //     closeness = 0.5 -> rate = halfway between the two
        float rate = cfg::kF3SuspicionFarRate
                   + (cfg::kF3SuspicionNearRate - cfg::kF3SuspicionFarRate) * closeness;

        if (sneaking_) {
            rate *= cfg::kF3SuspicionSneakMul;
        }

        suspicion_ += rate * deltaTime;

        if (suspicion_ >= 1.0f) {
            getCaught();
            return;
        }
    } else if (sightLostTimer_ > 0.0f) {
        // A short hold before it starts falling, so ducking behind a wall for
        // a split second does not wipe the meter.
        sightLostTimer_ = std::max(0.0f, sightLostTimer_ - deltaTime);
    } else {
        suspicion_ = std::max(0.0f, suspicion_ - cfg::kF3SuspicionDrain * deltaTime);
    }
}

// Advances a rock that is already in the air. There is no physics engine here
// and none is needed: we know where the rock started, where it will land, and
// how long it should take, so we just interpolate between the two. See
// drawRock() for the arc that gets drawn on top of that.
void StealthScene::updateRock(float deltaTime) {
    if (noiseTimer_ > 0.0f) {
        noiseTimer_ = std::max(0.0f, noiseTimer_ - deltaTime);
    }

    if (!rockInFlight_) {
        return;
    }

    rockElapsed_ += deltaTime;

    if (rockElapsed_ < rockDuration_) {
        return;
    }

    // Landed. The noise happens wherever it actually stopped - against a wall
    // if one got in the way, otherwise at the chosen spot.
    rockInFlight_ = false;
    raiseNoise(rockTo_);
}

//=============================================================================
// Drawing
//
// Nothing below this line is allowed to CHANGE the game. These functions
// only read state that update() already settled and turn it into shapes.
//
// Draw order matters for two reasons: things drawn later appear on top
// where they overlap, and see-through things (the vision cone, the aim
// marker, shadows) have to come after the solid things they sit over.
//
// Translucent shapes are wrapped in setDepthWrite(false) / (true). Without
// that they write to the depth buffer and hide whatever is behind them,
// which looks like a hole cut in the floor.
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
    // cameraViewProjection() rebuilds exactly this matrix for mouse picking.

    drawRoom();
    drawPath();
    drawGuard();
    drawAlertMark();
    drawVisionCone();   // after the guard, so it lies over the deck
    drawDoor();
    drawItems();
    drawNoise();
    drawAimMarker();
    drawRoary();
    drawRock();         // last, so it passes in front of whatever it flies over
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
    // left (-X): full height, framing the room.
    drawSlab(-halfX - thick * 0.5f, 0.0f, thick, spanZ, height);

    // right (+X): the same wall, but in two pieces with a gap for the exit
    // door. The gap is derived from the door constants rather than written out
    // again, so moving the door in Config.h moves the hole it sits in.
    const float gapMinZ = cfg::kF3DoorZ - cfg::kF3DoorWidth * 0.5f;
    const float gapMaxZ = cfg::kF3DoorZ + cfg::kF3DoorWidth * 0.5f;

    const float lowerSpan = gapMinZ - (-halfZ);
    const float upperSpan = halfZ - gapMaxZ;

    drawSlab(halfX + thick * 0.5f, -halfZ + lowerSpan * 0.5f,
             thick, lowerSpan, height);
    drawSlab(halfX + thick * 0.5f, gapMaxZ + upperSpan * 0.5f,
             thick, upperSpan, height);

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

// The "!" that pops over the guard's head when a rock grabs his attention.
// Two boxes - a stroke and a dot - with a surprising amount of space between
// them; see the note inside for why that is necessary rather than sloppy.
void StealthScene::drawAlertMark() {
    if (!guardIsDistracted()) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    const float headTop = cfg::kF3GuardBodyHeight + cfg::kF3GuardHeadSize;
    const float bob = std::sin(elapsedTime_ * cfg::kF3AlertBobSpeed)
                    * cfg::kF3AlertBobAmount;

    const float baseY = headTop + cfg::kF3AlertMarkY + bob;

    // Two boxes: the stroke, and the dot under it. Left unrotated so the mark
    // faces the camera, which never moves - a billboard would be the same
    // thing with more arithmetic.
    //
    // The gap between them has to be far wider than looks right on paper. The
    // camera is steeply overhead, which squashes vertical distances to roughly
    // half what the same distance measures across the floor, so a "normal"
    // gap closes up on screen and the mark reads as a plain bar.
    renderer.drawBoxAt(glm::vec3(guardPos_.x, baseY + 0.68f, guardPos_.z),
                       glm::vec3(0.20f, 0.70f, 0.20f),
                       cfg::kF3AlertMark, 1.0f, 0.85f);

    renderer.drawBoxAt(glm::vec3(guardPos_.x, baseY, guardPos_.z),
                       glm::vec3(0.22f, 0.22f, 0.22f),
                       cfg::kF3AlertMark, 1.0f, 0.85f);
}

void StealthScene::drawPath() {
    if (guardPath_.empty()) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    // Faint markers on the route he is walking. This is as much a debugging
    // aid as a player cue: if the pathfinder ever routes him through a wall,
    // the dots show it immediately.
    renderer.setDepthWrite(false);

    for (std::size_t i = guardPathNode_; i < guardPath_.size(); ++i) {
        const glm::vec3& node = guardPath_[i];
        const bool next = (i == guardPathNode_);

        renderer.drawCylinderAt(glm::vec3(node.x, 0.025f, node.z),
                                glm::vec3(next ? 0.44f : 0.30f, 0.02f,
                                          next ? 0.44f : 0.30f),
                                cfg::kF3GuardVisor, next ? 0.30f : 0.16f, 1.0f);
    }

    renderer.setDepthWrite(true);
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
    // Three states, in order of how much trouble the player is in: red once he
    // has actually been seen, amber while the guard is distracted and sweeping
    // toward a noise, calm cyan otherwise.
    glm::vec3 coneColor = cfg::kF3ConeCalm;
    float coneAlpha = cfg::kF3ConeAlpha;

    if (playerSeen_) {
        coneColor = cfg::kF3ConeAlert;
        coneAlpha = cfg::kF3ConeAlpha * 1.8f;
    } else if (guardIsDistracted()) {
        coneColor = cfg::kF3ConeAlerted;
        coneAlpha = cfg::kF3ConeAlpha * 1.35f;
    }

    renderer.drawPyramid(
        visionConeTransform(apex, guardFacingDeg_, range, width, cfg::kF3ConeHeight),
        coneColor, coneAlpha, 1.0f);
    renderer.setDepthWrite(true);
}

// The exit door in the right-hand wall, plus its stone frame. Red while it is
// locked, bright green with a pool of light on the floor once every paw has
// been collected. drawRoom() leaves a gap in the wall for this to sit in.
void StealthScene::drawDoor() {
    Renderer& renderer = ctx().renderer;

    const bool open = doorIsOpen();

    const float x      = cfg::kF3RoomHalfX + cfg::kF3WallThick * 0.5f;
    const float z      = cfg::kF3DoorZ;
    const float width  = cfg::kF3DoorWidth;
    const float height = cfg::kF3DoorHeight;
    const float thick  = cfg::kF3WallThick;

    const glm::vec3 panelColor = open ? cfg::kF3DoorOpen : cfg::kF3DoorLocked;

    // The panel filling the gap in the wall. Emissive once it is open, so the
    // way out is the brightest thing in the room and reads from anywhere.
    renderer.drawBoxAt(glm::vec3(x, height * 0.5f, z),
                       glm::vec3(thick * 0.92f, height, width),
                       panelColor, 1.0f, open ? 0.75f : 0.10f);

    // A stone frame around it, so the gap reads as a doorway rather than as a
    // missing piece of wall.
    const float jamb = 0.22f;
    for (int side = 0; side < 2; ++side) {
        const float offset = (side == 0 ? -1.0f : 1.0f) * (width * 0.5f + jamb * 0.5f);
        renderer.drawBoxAt(glm::vec3(x, cfg::kF3WallHeight * 0.5f, z + offset),
                           glm::vec3(thick * 1.08f, cfg::kF3WallHeight, jamb),
                           cfg::kF3DoorFrame);
    }

    // The lintel across the top, filling the wall above the shorter door.
    const float lintelHeight = cfg::kF3WallHeight - height;
    renderer.drawBoxAt(
        glm::vec3(x, height + lintelHeight * 0.5f, z),
        glm::vec3(thick * 1.08f, lintelHeight, width + jamb * 2.0f),
        cfg::kF3DoorFrame);

    // A pool of light spilling onto the deck in front of an open door. It is
    // the only cue that points at the exit from across the room.
    if (open) {
        renderer.setDepthWrite(false);
        renderer.drawCylinderAt(glm::vec3(cfg::kF3RoomHalfX - 0.9f, 0.025f, z),
                                glm::vec3(3.6f, 0.02f, 3.6f),
                                cfg::kF3DoorOpen, 0.16f, 1.0f);
        renderer.setDepthWrite(true);
    }
}

void StealthScene::drawAimMarker() {
    if (!aiming_ || !aimValid_) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    // Amber when a wall will stop the rock short, pale blue when the path is
    // clear. The marker is drawn at the IMPACT point either way, so what the
    // player sees is where the noise will happen.
    const glm::vec3 color = aimBlocked_ ? cfg::kF3MarkerBlocked
                                        : cfg::kF3MarkerOpen;

    renderer.setDepthWrite(false);

    // A breathing disc on the deck.
    const float pulse = 1.0f + std::sin(elapsedTime_ * 7.0f) * 0.07f;
    renderer.drawCylinderAt(glm::vec3(aimImpact_.x, 0.03f, aimImpact_.z),
                            glm::vec3(1.25f * pulse, 0.02f, 1.25f * pulse),
                            color, 0.30f, 1.0f);

    // Dots along the flight path, so the arc being blocked is obvious before
    // the throw rather than after it.
    const int dots = 9;
    for (int i = 1; i <= dots; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(dots + 1);

        const glm::vec3 ground = playerPos_ + (aimImpact_ - playerPos_) * t;
        const float height = cfg::kF3RockArcHeight * 4.0f * t * (1.0f - t);

        renderer.drawSphereAt(
            glm::vec3(ground.x, 0.25f + height, ground.z),
            glm::vec3(0.10f, 0.10f, 0.10f), color, 0.55f, 1.0f);
    }

    renderer.setDepthWrite(true);
}

// A rock in mid-flight: a small sphere following a parabola, with a shadow
// directly beneath it on the deck.
//
// The shadow is not decoration. From a camera this far away you genuinely
// cannot tell how high something is, so without a shadow the rock just looks
// like it is sliding along the floor.
void StealthScene::drawRock() {
    if (!rockInFlight_) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    const float t = std::min(1.0f, rockElapsed_ / std::max(rockDuration_, 1e-4f));

    // Straight line across the floor, parabola in the air.
    //
    // The arc comes from  4 * t * (1 - t),  which is worth understanding
    // because it is the cheapest useful curve in games:
    //
    //     t = 0.0  ->  4 * 0 * 1    = 0     (on the ground, just thrown)
    //     t = 0.5  ->  4 * 0.5*0.5  = 1     (exactly the top of the arc)
    //     t = 1.0  ->  4 * 1 * 0    = 0     (back on the ground, landed)
    //
    //            . - .
    //          .       .
    //        .           .
    //       0.0   0.5   1.0
    //
    // The 4 is what makes the peak exactly 1 instead of 0.25, which is why
    // kF3RockArcHeight is the REAL peak height in world units rather than some
    // number that merely influences it. No gravity, no velocity, no physics
    // step - just one multiply, and it looks right.
    const glm::vec3 ground = rockFrom_ + (rockTo_ - rockFrom_) * t;
    const float height = cfg::kF3RockArcHeight * 4.0f * t * (1.0f - t);

    const glm::vec3 position(ground.x, 0.28f + height, ground.z);

    renderer.drawSphereAt(position,
                          glm::vec3(cfg::kF3RockRadius * 2.0f),
                          cfg::kF3RockColor);

    // Its shadow, so the height reads from a camera this far away.
    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(ground.x, 0.02f, ground.z),
                            glm::vec3(0.34f, 0.02f, 0.34f),
                            cfg::kF3ShadowColor, 0.40f, 1.0f);
    renderer.setDepthWrite(true);
}

void StealthScene::drawNoise() {
    if (noiseTimer_ <= 0.0f) {
        return;
    }

    Renderer& renderer = ctx().renderer;

    // One ring expanding to the full hearing radius and fading as it goes.
    // When guards can hear, the moment the ring reaches one is the moment he
    // reacts, so this is a readout of the mechanic rather than decoration.
    const float progress = 1.0f - (noiseTimer_ / cfg::kF3NoiseDuration);
    const float radius = cfg::kF3NoiseRadius * progress;
    const float alpha = 0.34f * (1.0f - progress);

    renderer.setDepthWrite(false);
    renderer.drawCylinderAt(glm::vec3(noisePos_.x, 0.04f, noisePos_.z),
                            glm::vec3(radius * 2.0f, 0.02f, radius * 2.0f),
                            cfg::kF3NoiseColor, alpha, 1.0f);
    renderer.setDepthWrite(true);
}

// The paw pickups. Each one hovers, bobs and slowly spins, with a faint halo
// on the deck underneath so it catches the eye from across the room.
//
// Collected items are simply skipped, which is all "disappearing" needs to
// mean - there is nothing to delete, because the positions live in Config.h
// and only the collected FLAGS live here.
void StealthScene::drawItems() {
    Renderer& renderer = ctx().renderer;

    const float k = cfg::kF3ItemScale;

    for (int i = 0; i < cfg::kF3ItemCount; ++i) {
        if (itemCollected_[static_cast<std::size_t>(i)]) {
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
    // The panther himself lives in scenes/Characters.cpp, because the boss
    // level draws exactly the same character. All this scene has to do is say
    // where he is and what he is doing.
    // The leading :: matters: without it the compiler finds this member
    // function again and the call recurses into itself forever.
    ::drawRoary(ctx().renderer, playerPos_, facingDeg_, walkCycle_,
                moveDir_ != glm::vec2(0.0f), cfg::kF3PlayerScale);
}

// Converts a measurement written in "design pixels" into real screen pixels.
//
// Every UI number in this file is written as if the window were 800 pixels
// tall. On your Retina display the framebuffer is actually 1600 tall, so
// without this every label and bar would come out at half the size it should
// be. Wrap any UI measurement in px() and it works on both.
float StealthScene::px(float designPixels) const {
    return designPixels * uiScale_;
}

void StealthScene::drawSuspicionMeter() {
    TextRenderer& text = ctx().renderer.text();

    const float width   = static_cast<float>(ctx().framebufferWidth);
    const float centerX = width * 0.5f;
    const float top     = px(132.0f);

    // CAUGHT takes over the space the meter uses, since the meter has just
    // been zeroed and there is nothing to show there anyway.
    if (caughtFlashTimer_ > 0.0f) {
        const std::string caught = "CAUGHT! BACK TO THE START";
        const float textWidth =
            TextRenderer::measureWidth(caught, px(cfg::kFloorNameScale * 0.75f));

        text.fillRect(centerX - textWidth * 0.5f - px(14.0f), top - px(9.0f),
                      textWidth + px(28.0f),
                      TextRenderer::measureHeight(px(cfg::kFloorNameScale * 0.75f))
                          + px(18.0f),
                      cfg::kBgBottom, 0.80f);

        text.drawCentered(caught, centerX, top,
                          px(cfg::kFloorNameScale * 0.75f), cfg::kF3ConeAlert);
        return;
    }

    // Hidden entirely at zero: a permanent empty bar is noise, and its
    // appearing is itself the warning that something has noticed you.
    if (suspicion_ <= 0.001f) {
        return;
    }

    const float barWidth  = px(cfg::kF3MeterWidth);
    const float barHeight = px(cfg::kF3MeterHeight);
    const float barX      = centerX - barWidth * 0.5f;

    // The map fills nearly the whole frame, so the meter always sits over the
    // deck. A dark panel behind it keeps both the bar and its label readable
    // whatever colour of floor happens to be underneath.
    text.fillRect(barX - px(12.0f), top - px(9.0f),
                  barWidth + px(24.0f), barHeight + px(38.0f),
                  cfg::kBgBottom, 0.78f);

    // Track, then the fill over it.
    text.fillRect(barX - px(2.0f), top - px(2.0f),
                  barWidth + px(4.0f), barHeight + px(4.0f),
                  cfg::kF3Trim, 0.55f);

    const float filled = std::clamp(suspicion_, 0.0f, 1.0f);
    const glm::vec3 fillColor = glm::mix(cfg::kF3MeterLow, cfg::kF3MeterHigh, filled);

    text.fillRect(barX, top, barWidth * filled, barHeight, fillColor, 0.95f);

    // The label doubles as the reason the meter is moving: filling means he
    // has eyes on you right now, fading means you got out of sight in time.
    const char* label = playerSeen_ ? "SPOTTED" : "LOSING INTEREST";

    text.drawCentered(label, centerX, top + barHeight + px(6.0f),
                      px(cfg::kHintScale),
                      playerSeen_ ? cfg::kF3MeterHigh : cfg::kTextDim);
}

// All the flat on-screen text and panels: the floor name, the paw and rock
// counters, the door messages and the control hints.
//
// Everything between text.begin() and text.end() is drawn in 2D screen pixels
// with (0,0) at the TOP-LEFT and y growing downward - a different coordinate
// system from the 3D world above, so do not mix the two up. begin() also turns
// depth testing off, which is what keeps the HUD on top of the scene.
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

    // The door's state, so the player knows what the paws are FOR before they
    // have walked all the way across the level to find out.
    if (lockedFlashTimer_ > 0.0f) {
        const std::string lockedMessage =
            "THE DOOR IS LOCKED - FIND ALL " + std::to_string(cfg::kF3ItemCount)
            + " PAWS";
        text.drawCentered(lockedMessage, centerX, bottom - px(196.0f),
                          px(cfg::kHintScale), cfg::kTextLocked);
    } else if (doorIsOpen()) {
        text.drawCentered("THE DOOR IS OPEN - GET OUT",
                          centerX, bottom - px(196.0f),
                          px(cfg::kHintScale), cfg::kTextComplete);
    }

    drawSuspicionMeter();

    // Shows the sneak state is actually reaching the simulation. This becomes
    // a real noise indicator once guards can hear, in step 6.
    text.drawCentered(sneaking_ ? "SNEAKING" : "WALKING",
                      centerX, bottom - px(126.0f),
                      px(cfg::kHintScale),
                      sneaking_ ? cfg::kF3Light : cfg::kTextDim);

    // Rock counter, under the paw counter in the top right.
    const std::string rockLabel = "ROCKS " + std::to_string(rocksLeft_)
                                + "/" + std::to_string(cfg::kF3RockCount);
    text.draw(rockLabel,
              static_cast<float>(width) - px(24.0f)
                  - TextRenderer::measureWidth(rockLabel, px(cfg::kHintScale)),
              px(54.0f), px(cfg::kHintScale),
              rocksLeft_ > 0 ? cfg::kF3RockColor : cfg::kTextLocked);

    // While aiming, say what the marker colour means. Without this the player
    // has to work out for themselves why it sometimes turns amber.
    if (aiming_) {
        text.drawCentered(aimBlocked_ ? "A WALL WILL STOP IT THERE"
                                      : "CLICK TO THROW     E TO CANCEL",
                          centerX, bottom - px(160.0f), px(cfg::kHintScale),
                          aimBlocked_ ? cfg::kF3MarkerBlocked : cfg::kF3MarkerOpen);
    }

    text.drawCentered("WASD - MOVE     SHIFT - SNEAK     E - AIM A ROCK",
                      centerX, bottom - px(90.0f),
                      px(cfg::kHintScale), cfg::kTextPrimary);

    text.drawCentered("ESC - BACK TO THE TOWER     C - DEBUG COMPLETE",
                      centerX, bottom - px(54.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
