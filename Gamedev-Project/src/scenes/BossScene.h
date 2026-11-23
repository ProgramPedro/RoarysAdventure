#pragma once

// The top floor, Rival Showdown: a dodging fight against Rocky the bull. Roary
// can only move and dash, and Rocky is hurt by being baited into the wall and then
// touched while he is stunned. Roary has hearts, Rocky has health, and the tuning
// lives in the cfg::kF4 values in Config.h.
//
// Dashing past Rocky while he is charging PARRIES him: the next punish does
// double damage. It is the brave read - the dash is also the only way out of a
// charge, so running at the bull means giving up the escape to earn it.
//
// Half way through, once Rocky is down to cfg::kF4IbisJoinHealth, Sebastian the
// ibis joins him and the two attack at the same time from then on. Sebastian
// cannot be damaged - Rocky is the ring leader, so the floor ends when HE does
// - and his attack is an aerial slam that marks a circle on the deck before it
// lands.


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
    static const int kFloorIndex = 3;

    enum class RockyState {
        Waiting,
        Telegraph,
        Charging,
        Stunned,
        Recover,
        Beaten
    };

    void returnToMenu();
    void resetFight();

    // Sebastian's states. He runs his own machine on his own timer, which is
    // what makes the two bosses overlap rather than take turns.
    enum class IbisState {
        Away,       // not in the fight yet
        Hover,      // circling above, drifting after Roary
        Marking,    // the circle is on the deck and he is about to drop
        Slamming,   // falling
        Recover     // sat on the deck, then climbing back up
    };

    void updateRocky(float deltaTime);
    void updateIbis(float deltaTime);
    void setIbisState(IbisState state, float duration);

    // The moment he touches down: anybody still inside the circle is hit.
    void checkSlamContact();

    // True once he has joined, so the drawing and the HUD can ask.
    bool ibisInFight() const { return ibisState_ != IbisState::Away; }
    void setRockyState(RockyState state, float duration);
    void checkChargeContact();
    void tryPunish();

    // Checks whether this frame's dash is passing close enough to a charging
    // Rocky to count as a parry. Called from the charging state, so he is
    // always actually charging when it runs.
    void checkParry();

    void drawIbis();
    void drawSlamMark();

    // The ring drawn on the deck around a stunned Rocky. Added after round 1
    // of playtesting, where two of three testers had to be told that touching
    // him while he is down is what damages him.
    void drawOpeningRing();
    void takeHit();

    void movePlayer(float deltaTime);
    void startDash();
    void updateDash(float deltaTime);
    glm::vec3 clampToArena(const glm::vec3& position, float radius) const;

    bool isInvulnerable() const {
        return dashTimer_ > 0.0f || playerInvulnTimer_ > 0.0f;
    }

    void drawArena();
    void drawRocky();
    void drawPlayer();
    void drawUi();

    float px(float designPixels) const;

    glm::vec3 playerPos_{0.0f};
    glm::vec2 moveDir_{0.0f, 0.0f};
    float     facingDeg_         = 180.0f;
    int       hearts_            = cfg::kF4MaxHearts;
    float     playerInvulnTimer_ = 0.0f;

    // Set by a parry and spent by the next punish, so the damage bonus is
    // earned once and used once.
    bool  parryReady_     = false;
    float parryGlowTimer_ = 0.0f;   // how long Rocky stays orange
    float parryTextTimer_ = 0.0f;   // how long PARRY! stays on screen

    // --- Sebastian -----------------------------------------------------------
    IbisState ibisState_ = IbisState::Away;
    float     ibisTimer_ = 0.0f;

    // y carries his height above the deck, so the same vector drives both
    // where he is and how far he still has to fall.
    glm::vec3 ibisPos_{0.0f};
    float     ibisFacingDeg_ = 0.0f;

    // Where the slam will land. Taken once when the telegraph starts so the
    // circle cannot chase Roary around.
    glm::vec3 ibisMarkPos_{0.0f};

    float ibisEntryTextTimer_ = 0.0f;

    float     dashTimer_    = 0.0f;
    float     dashCooldown_ = 0.0f;
    glm::vec2 dashDir_{0.0f, 1.0f};

    RockyState rockyState_     = RockyState::Waiting;
    float      rockyTimer_     = 0.0f;
    int        rockyHealth_    = cfg::kF4RockyHealth;
    glm::vec3  rockyPos_{0.0f};
    float      rockyFacingDeg_ = 0.0f;
    glm::vec2  chargeDir_{0.0f, 1.0f};

    float outcomeTimer_ = 0.0f;
    bool  victory_      = false;

    float uiScale_ = 1.0f;
};
