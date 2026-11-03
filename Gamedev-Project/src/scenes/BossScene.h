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
//   [x] Rocky the bull: telegraph, charge, crash into the parapet
//   [x] punish windows - touch a stunned boss to damage it
//   [x] beating Rocky finishes the floor
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

    //-------------------------------------------------------------------------
    // ROCKY'S STATE MACHINE
    //
    // He is only ever doing ONE of these, and each one leads to the next. A
    // single enum plus a single timer is the whole of his brain - which is all
    // a readable boss needs. Anything more and the player cannot predict him.
    //-------------------------------------------------------------------------
    enum class RockyState {
        Waiting,    // squaring up, tracking Roary slowly
        Telegraph,  // aim locked, lane shown on the deck, about to go
        Charging,   // committed, travelling fast, cannot steer
        Stunned,    // hit the parapet; open to a punish
        Recover,    // shaking it off, briefly untouchable
        Beaten      // out of health; the floor is won
    };

    void updateRocky(float deltaTime);

    // Moves him to the next state and resets his timer with it, so the two can
    // never disagree about what he is doing.
    void setRockyState(RockyState state, float duration);

    // Roary touching Rocky while he is stunned: the only way to damage him.
    void tryPunish();

    // Rocky's body reaching Roary mid-charge.
    void checkChargeContact();

    // Roary loses a heart, or the fight is lost.
    void takeHit();

    // Reverts the arena to the opening state after a loss.
    void resetFight();

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
    // Two things can protect Roary: the dash's own window, and the brief grace
    // after taking a hit. Rocky asks this before landing one.
    bool isInvulnerable() const {
        return dashTimer_ > 0.0f || playerInvulnTimer_ > 0.0f;
    }

    // 0 when the dash has just been used, 1 when it is ready again. Drives the
    // readiness pip in the HUD.
    float dashCharge() const;

    // --- drawing -------------------------------------------------------------
    void drawArena();
    void drawRocky();
    void drawChargeLane();
    void drawPlayer();
    void drawUi();

    // Converts a design-pixel measurement into real framebuffer pixels, so the
    // UI is the same apparent size on a Retina display as on a normal one.
    float px(float designPixels) const;

    // --- state ---------------------------------------------------------------
    glm::vec3 playerPos_{0.0f};
    glm::vec2 moveDir_{0.0f, 0.0f};
    float     facingDeg_ = 180.0f;

    // --- dash state ----------------------------------------------------------
    // dashTimer_ counts the dash itself down and is also the invincibility
    // window. dashCooldown_ counts down separately and is what gates the next
    // press - it starts at the full cooldown the moment the dash BEGINS, so
    // the cycle is a clean five seconds end to end.
    float     dashTimer_    = 0.0f;
    float     dashCooldown_ = 0.0f;
    glm::vec2 dashDir_{0.0f, 1.0f};

    // (A ghost trail used to be drawn behind a dash. It is on hold with the
    // rest of the animation work; the dash itself is unaffected.)

    // --- Rocky ---------------------------------------------------------------
    RockyState rockyState_   = RockyState::Waiting;
    float      rockyTimer_   = 0.0f;
    int        rockyHealth_  = cfg::kF4RockyHealth;
    glm::vec3  rockyPos_{0.0f};
    float      rockyFacingDeg_ = 0.0f;

    // The direction locked in at the end of the telegraph. He charges along
    // this no matter where Roary goes afterwards - that commitment is what
    // makes the attack dodgeable.
    glm::vec2  chargeDir_{0.0f, 1.0f};

    // Counts down after Roary is hit, so one charge cannot cost two hearts.
    float playerInvulnTimer_ = 0.0f;

    // Non-zero while a win or loss banner is up and the level is waiting
    // before it reacts.
    float outcomeTimer_ = 0.0f;
    bool  victory_      = false;

    int   hearts_      = cfg::kF4MaxHearts;
    float elapsedTime_ = 0.0f;
    float uiScale_     = 1.0f;
};
