#pragma once
// BossScene - Top floor, "Rival Showdown"
// A dodging fight against Rocky the bull. Roary can only move and dash; Rocky
// is damaged by touching him while he's stunned. Tuning values are in Config.h (kF4).

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

    // Rocky is always in exactly one of these states
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

    void updateRocky(float deltaTime);
    void setRockyState(RockyState state, float duration);
    void checkChargeContact();
    void tryPunish();
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

    // Roary
    glm::vec3 playerPos_{0.0f};
    glm::vec2 moveDir_{0.0f, 0.0f};
    float     facingDeg_         = 180.0f;
    int       hearts_            = cfg::kF4MaxHearts;
    float     playerInvulnTimer_ = 0.0f;

    // Dash
    float     dashTimer_    = 0.0f;
    float     dashCooldown_ = 0.0f;
    glm::vec2 dashDir_{0.0f, 1.0f};

    // Rocky
    RockyState rockyState_     = RockyState::Waiting;
    float      rockyTimer_     = 0.0f;
    int        rockyHealth_    = cfg::kF4RockyHealth;
    glm::vec3  rockyPos_{0.0f};
    float      rockyFacingDeg_ = 0.0f;
    glm::vec2  chargeDir_{0.0f, 1.0f};

    // Win / lose banner
    float outcomeTimer_ = 0.0f;
    bool  victory_      = false;

    float uiScale_ = 1.0f;
};
