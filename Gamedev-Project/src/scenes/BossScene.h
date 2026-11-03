#pragma once
//=============================================================================
// BossScene - THE TOP FLOOR, "RIVAL SHOWDOWN".
//
// A dodging fight on the open castle roof against the three rival mascots.
// Roary has no attack button: he moves, and he dashes, and that is all. You
// damage a boss by dodging its attack and touching it while it is recovering.
//
// BUILT IN STEPS. What works so far:
//   [x] the round arena, Roary walking it, and the dash
//   [ ] Rocky the bull: telegraph, charge, crash into the parapet
//   [ ] punish windows - touch a stunned boss to damage it
//   [ ] Owlsley and Sebastian in the attack rotation
//   [ ] freeing Pawlina when all three are beaten
//
// CONTROLS
//   W A S D / arrows   move
//   Space              dash - a fast burst, invincible while it lasts
//   Esc                back to the tower
//   C                  DEBUG: mark this floor complete
//
// COORDINATES: the arena is a circle centred on the origin, lying on the XZ
// plane with +Y up. There are no walls, so "am I inside the arena" is a single
// distance-from-the-centre test. Every size lives in the TOP FLOOR GAMEPLAY
// section of Config.h.
//=============================================================================

#include "core/Config.h"
#include "core/Scene.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <array>

class BossScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return "BossScene"; }

private:
    // The top floor is the fourth, so index 3.
    static constexpr int kFloorIndex = 3;

    void returnToMenu();

    // --- simulation ----------------------------------------------------------
    void movePlayer(float deltaTime);

    // Starts a dash, if one is allowed right now. Doing nothing is the correct
    // response to pressing space on cooldown - no queueing, no buffering.
    void startDash();

    // Counts the dash and its cooldown down.
    void updateDash(float deltaTime);

    // Keeps a position inside the round arena. The whole of this level's
    // collision, in one function - compare the six-case wall handling the
    // stealth floor needed.
    glm::vec3 clampToArena(const glm::vec3& position, float radius) const;

    // True while the dash's invincibility frames are active. The bosses will
    // ask this before landing a hit.
    bool isInvulnerable() const { return dashTimer_ > 0.0f; }

    // 0 when the dash has just been used, 1 when it is ready again. Drives the
    // readiness pip in the HUD.
    float dashCharge() const;

    // --- drawing -------------------------------------------------------------
    void drawArena();
    void drawPlayer();
    void drawUi();

    // Converts a design-pixel measurement into real framebuffer pixels, so the
    // UI is the same apparent size on a Retina display as on a normal one.
    float px(float designPixels) const;

    // --- state ---------------------------------------------------------------
    glm::vec3 playerPos_{0.0f};
    glm::vec2 moveDir_{0.0f, 0.0f};
    float     facingDeg_ = 180.0f;
    float     walkCycle_ = 0.0f;

    // --- dash state ----------------------------------------------------------
    // dashTimer_ counts the dash itself down and is also the invincibility
    // window. dashCooldown_ counts down separately and is what gates the next
    // press - it starts at the full cooldown the moment the dash BEGINS, so
    // the cycle is a clean five seconds end to end.
    float     dashTimer_    = 0.0f;
    float     dashCooldown_ = 0.0f;
    glm::vec2 dashDir_{0.0f, 1.0f};

    // Recent positions, newest first, for the ghost trail. A plain array
    // rather than a queue: it never grows, and the oldest entry just falls off
    // the end each time a new one is pushed in.
    std::array<glm::vec3, cfg::kF4DashTrailCount> dashTrail_{};
    int   dashTrailCount_ = 0;
    float dashTrailTimer_ = 0.0f;

    int   hearts_      = cfg::kF4MaxHearts;
    float elapsedTime_ = 0.0f;
    float uiScale_     = 1.0f;
};
