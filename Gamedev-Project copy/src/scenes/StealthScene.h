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
//   [~] a guard, standing still - patrol routes still to come
//   [~] a vision cone, and whether it can actually see Roary
//   [ ] a suspicion meter, and failing the level when it fills
//   [x] key items to pick up
//   [x] the locked door, which is the win condition
//   [ ] throwable rocks that distract guards
//
// CONTROLS
//   W A S D / arrows   move
//   Shift              sneak - slower, and quieter once guards can hear
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

    // Advances only while he is actually walking, so the bounce stops when he
    // does instead of bobbing on the spot.
    float walkCycle_ = 0.0f;

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

    // Counts down after walking into a door that will not open, so the LOCKED
    // message flashes rather than the attempt doing nothing visible. Same
    // trick the level-select screen uses for locked floors.
    float lockedFlashTimer_ = 0.0f;

    bool  sneaking_    = false;
    float elapsedTime_ = 0.0f;
    float uiScale_     = 1.0f;
};
