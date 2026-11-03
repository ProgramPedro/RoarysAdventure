#pragma once
//=============================================================================
// StealthScene - FLOOR 3, "SILENT PAWS".
//
// Sneak through a guarded room, collect the key items, and use them to unlock
// the door to the next floor. Rocks can be thrown to pull a guard's attention
// away from the route you actually want to take.
//
// BUILT IN STEPS. What works so far:
//   [x] the scene exists, is reachable, and can report the floor complete
//   [x] the room, and Roary walking around it with wall collision
//   [x] a guard patrolling a route, pathfinding around the walls
//   [x] a vision cone, and whether it can actually see Roary
//   [x] a suspicion meter, and failing the level when it fills
//   [x] key items to pick up
//   [x] the locked door, which is the win condition
//   [x] throwable rocks that distract the guard
//
// CONTROLS
//   W A S D / arrows   move
//   Shift              sneak - slower, and quieter once guards can hear
//   E                  aim a rock (press again to cancel)
//   Mouse + click      while aiming: pick the spot and throw
//   Esc                back to the tower
//   C                  DEBUG: mark this floor complete, which unlocks floor 4
//
// COORDINATES: the room lies on the XZ plane with +Y up, centered on the
// origin. -Z is the far wall, +Z is the near one. Every size lives in the
// FLOOR 3 GAMEPLAY section of Config.h.
//=============================================================================

#include "core/Config.h"   // kF3ItemCount sizes the array below
#include "core/Scene.h"

#include <array>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class StealthScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return "StealthScene"; }

private:
    // This floor is the third one, so index 2. Used for markCompleted() and to
    // look up the floor's name and description in cfg::kFloorInfo.
    static constexpr int kFloorIndex = 2;

    void returnToMenu();

    // --- simulation ----------------------------------------------------------
    // Moves Roary by this frame's input and pushes him back out of the walls.
    void movePlayer(float deltaTime);

    // Keeps a position inside the outer walls, allowing for how wide Roary is.
    // Split out because the guards and the thrown rocks will want the same test.
    glm::vec3 clampToRoom(const glm::vec3& position, float radius) const;

    // --- rocks ---------------------------------------------------------------
    // Where the cursor is pointing on the floor, and where a rock thrown there
    // would actually come to rest. Called every frame while aiming.
    void updateAim(const Input& input);

    // Launches a rock at the current aim. Does nothing if none are left.
    void throwRock();

    // --- navigation ----------------------------------------------------------
    // Builds the grid of walkable deck cells the pathfinder searches. Run once
    // from init(); the level's geometry never changes after that.
    void buildNavGrid();

    // Is a guard-sized body standing at this cell clear of every wall?
    bool navCellOpen(int ix, int iz) const;

    // Grid <-> world. navCellFor() returns false for a point off the grid.
    glm::vec3 navCellCenter(int ix, int iz) const;
    bool navCellFor(const glm::vec3& position, int& outX, int& outZ) const;

    // DIJKSTRA. Shortest walkable route from `from` to `to`, as a list of
    // world points. Returns false when no route exists. Costs are real
    // distances - 1 for a step, sqrt(2) for a diagonal - so the result is the
    // genuinely shortest path rather than the one with the fewest cells.
    bool findPath(const glm::vec3& from, const glm::vec3& to,
                  std::vector<glm::vec3>& outPath) const;

    // True when a body of `radius` can sweep from a to b without clipping a
    // wall. Used to straighten the raw grid path, which is otherwise a
    // staircase the guard would visibly jink along.
    bool sweepIsClear(const glm::vec3& from, const glm::vec3& to,
                      float radius) const;

    // Sends the guard to his current patrol point, pathfinding around walls.
    void repathGuard();

    // --- suspicion -----------------------------------------------------------
    void updateSuspicion(float deltaTime);

    // Resets Roary and the guard after a full meter. Collected paws are KEPT:
    // losing them would turn one mistake into replaying the whole level.
    void getCaught();

    // Raises a noise at `position`: starts the ring, and distracts any guard
    // close enough to hear it. Everything that makes a sound should go through
    // here rather than setting the fields directly.
    void raiseNoise(const glm::vec3& position);

    // Turns the guard toward whatever has his attention, and counts his
    // distraction down. He never changes position - there is no patrol yet.
    void updateGuard(float deltaTime);

    // True while the exclamation mark is up.
    bool guardIsDistracted() const { return guardAlertTimer_ > 0.0f; }

    // Advances a rock already in the air, and raises the noise when it lands.
    void updateRock(float deltaTime);

    // The camera's combined view-projection. Built here rather than read back
    // from the Renderer because handleInput() runs BEFORE render() sets it,
    // and aiming needs a matrix that matches this frame, not the last one.
    glm::mat4 cameraViewProjection() const;

    // Turns a cursor position in framebuffer pixels into the point on the deck
    // underneath it. False when the cursor is above the horizon and the ray
    // never meets the floor.
    bool screenToFloor(const glm::vec2& screenPos, glm::vec3& outFloor) const;

    // The earliest point along `from` -> `to` where an interior wall gets in
    // the way. Returns false if the path is clear. lineCrossesWall() is this
    // with the distance thrown away.
    static bool firstWallHit(const glm::vec3& from, const glm::vec3& to,
                             float& outT);

    // True once every paw has been collected and the door will open.
    bool doorIsOpen() const;

    // Checks whether Roary has reached the door, and finishes the level if it
    // is open. Starts the LOCKED flash if it is not.
    void checkDoor();

    // Ends the level: marks floor 3 complete, which unlocks floor 4, and goes
    // back to the tower. The one call the whole level has been building to.
    void finishLevel();

    // Collects any item Roary is standing close enough to.
    void collectNearbyItems();

    // How many paws he is carrying.
    int collectedCount() const;

    // True when the guard can see Roary right now: in range, inside the cone's
    // half-angle, and with no wall between them.
    bool guardCanSeePlayer() const;

    // Whether the straight line from `from` to `to` crosses any interior wall.
    // This is the line-of-sight test, and it is why a guard cannot see through
    // a divider. Static because it is pure geometry over the Config table.
    static bool lineCrossesWall(const glm::vec3& from, const glm::vec3& to);

    // Pushes a position back out of any interior wall it has ended up inside,
    // moving it on ONE axis only. Called once per axis after moving on that
    // axis: resolving the two separately is what lets Roary slide along a wall
    // he is pressed against instead of sticking to it.
    void resolveInnerWalls(glm::vec3& position, int axis, float delta,
                           float radius) const;

    // --- drawing -------------------------------------------------------------
    void drawRoom();
    void drawRoary();
    void drawGuard();
    void drawVisionCone();
    void drawItems();
    void drawDoor();
    void drawAimMarker();
    void drawRock();
    void drawNoise();
    void drawAlertMark();
    void drawPath();
    void drawSuspicionMeter();
    void drawUi();

    // Converts a design-pixel measurement into real framebuffer pixels, so the
    // UI is the same apparent size on a Retina display as on a normal one.
    // See "A note on UI sizing" in README.md.
    float px(float designPixels) const;

    // --- state ---------------------------------------------------------------
    // Roary's position on the deck. Y stays 0: he does not jump.
    glm::vec3 playerPos_{0.0f};

    // This frame's movement direction in XZ, already normalized. Zero when he
    // is standing still, which is what the walk bounce and the HUD read.
    glm::vec2 moveDir_{0.0f, 0.0f};

    // Which way he FACES, in degrees about Y. Eased toward his heading so he
    // turns rather than snapping, and held when he stops so he does not spin
    // back to a default direction the moment the key comes up.
    float facingDeg_ = 180.0f;

    // --- the guard -----------------------------------------------------------
    // One guard, standing where Config.h puts him. He does not move or see
    // anything yet. Kept as plain members rather than a Guard struct until
    // there is a second one and some behaviour worth grouping.
    glm::vec3 guardPos_{0.0f};
    float     guardFacingDeg_ = 0.0f;

    // --- key items -----------------------------------------------------------
    // One flag per entry in cfg::kF3ItemPositions. Parallel to that table
    // rather than a copy of it, so the positions stay in Config.h where the
    // rest of the level layout lives.
    std::array<bool, cfg::kF3ItemCount> itemCollected_{};

    // Recomputed once per update and then read by the drawing code, so the
    // cone's colour and the HUD cannot disagree about what the guard sees.
    bool playerSeen_ = false;

    // --- rock state ----------------------------------------------------------
    bool      aiming_      = false;
    bool      aimValid_    = false;  // cursor is actually over the floor
    bool      aimBlocked_  = false;  // a wall stops the rock short
    glm::vec3 aimTarget_{0.0f};      // where the player is pointing
    glm::vec3 aimImpact_{0.0f};      // where the rock would really land

    int rocksLeft_ = 0;

    // One rock in the air at a time. More would need a container and buys
    // nothing: the player throws, watches, then throws again.
    bool      rockInFlight_ = false;
    glm::vec3 rockFrom_{0.0f};
    glm::vec3 rockTo_{0.0f};
    float     rockElapsed_  = 0.0f;
    float     rockDuration_ = 0.0f;

    // The most recent noise, as a position and a countdown. This is the hook
    // guards will read once they can investigate.
    glm::vec3 noisePos_{0.0f};
    float     noiseTimer_ = 0.0f;

    // Counts down after walking into a door that will not open, so the LOCKED
    // message flashes rather than the attempt doing nothing visible. Same
    // trick the level-select screen uses for locked floors.
    float lockedFlashTimer_ = 0.0f;

    // --- patrol state --------------------------------------------------------
    // The nav grid: one flag per cell, row-major as [ix * navCountZ_ + iz].
    std::vector<char> navOpen_;
    int navCountX_ = 0;
    int navCountZ_ = 0;

    // The route he is currently walking, and how far along it he is. Empty
    // when he has nowhere to be.
    std::vector<glm::vec3> guardPath_;
    std::size_t guardPathNode_ = 0;

    int   patrolIndex_     = 0;
    float guardPauseTimer_ = 0.0f;

    // --- suspicion state -----------------------------------------------------
    float suspicion_        = 0.0f;  // 0 unseen, 1 caught
    float sightLostTimer_   = 0.0f;  // grace before the meter starts draining
    float caughtFlashTimer_ = 0.0f;

    // The facing he returns to once he loses interest. Stored rather than read
    // from Config so that a patrolling guard can later set it to "whatever I
    // was doing before the rock" without this code changing.
    float     guardHomeFacingDeg_ = 0.0f;
    float     guardAlertTimer_    = 0.0f;
    glm::vec3 guardAlertTarget_{0.0f};

    bool  sneaking_    = false;
    float elapsedTime_ = 0.0f;
    float uiScale_     = 1.0f;
};
