#pragma once
//=============================================================================
// PlatformerScene - Floor 1, "Platform Peril".
//
// A 3D platformer: run, jump and climb a tower of platforms to the goal at the
// top, collecting coins on the way.
//
// CONTROLS
//   WASD / arrows   move
//   Space           jump (hold for a full jump, tap for a hop)
//   R               respawn at the last platform you stood on
//
// MECHANICS
//   - land on a Walker to squash it and bounce off; Spikers must be dodged
//   - touching an enemy from the side costs a life and knocks you back
//   - punch an item block from underneath and a coin pops out
//   - bounce pads throw you far higher than a jump can reach
//   Esc             back to the tower
//   C               DEBUG: mark the floor complete
//
// The level layout lives in PlatformerLevel.h and the feel is tuned from
// cfg::plat in Config.h. Neither needs any of this file to be understood.
//=============================================================================

#include "core/Collision.h"
#include "core/Scene.h"
#include "scenes/PlatformerLevel.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

class PlatformerScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return "PlatformerScene"; }

private:
    // A moving platform's live state. The definition it came from is static
    // data; this is where it actually is right now.
    struct MovingPlatform {
        glm::vec3 from{0.0f};
        glm::vec3 to{0.0f};
        glm::vec3 size{1.0f};
        float speed = 1.0f;
        float travel = 0.0f;    // 0..1 along from->to
        int direction = 1;      // +1 heading to `to`, -1 coming back
        glm::vec3 center{0.0f};
        glm::vec3 lastDelta{0.0f};
    };

    // A patrolling enemy. `position` is where its feet are.
    struct Enemy {
        level1::EnemyKind kind = level1::EnemyKind::Walker;
        glm::vec3 position{0.0f};
        float minX = 0.0f;
        float maxX = 0.0f;
        float speed = 1.0f;
        int direction = 1;
        bool alive = true;
        float squashTimer = 0.0f;   // counts down the squash animation
    };

    // The coin that flies out of an item block when you hit it.
    struct PoppedCoin {
        glm::vec3 origin{0.0f};
        float timer = 0.0f;
    };

    void respawn();
    void restartFromStart();
    void updateMovingPlatforms(float deltaTime);
    void rebuildSolids();
    void updatePlayer(float deltaTime);
    void updateCamera(float deltaTime, bool snap);
    void collectCoins();
    void checkGoal();
    void updateEnemies(float deltaTime);
    void resolveEnemyContacts();
    void takeDamage(const glm::vec3& source);
    void punchItemBlockAbove();
    void checkBouncePads();
    void updatePoppedCoins(float deltaTime);

    Aabb enemyBox(const Enemy& enemy) const;

    void drawWorld();
    void drawEnemies();
    void drawHud();

    // Finds the surface directly beneath Roary so a drop shadow can be drawn
    // on it. Without the shadow, judging a jump in 3D is guesswork.
    bool findGroundShadow(float& outY) const;

    // --- player ---------------------------------------------------------------
    Aabb player_{};
    glm::vec3 velocity_{0.0f};
    bool grounded_ = false;
    bool jumping_ = false;
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    float facingYaw_ = 0.0f;
    glm::vec3 lastSafePosition_{0.0f};

    // --- input captured this frame -------------------------------------------
    glm::vec2 moveInput_{0.0f};
    bool jumpHeld_ = false;

    // --- world ----------------------------------------------------------------
    std::vector<Aabb> staticSolids_;   // built once from the level data
    std::vector<MovingPlatform> movingPlatforms_;
    std::vector<Aabb> solids_;         // statics + moving, rebuilt each frame
    std::vector<bool> coinTaken_;
    std::vector<bool> itemBlockUsed_;
    std::vector<Enemy> enemies_;
    std::vector<PoppedCoin> poppedCoins_;
    std::vector<float> bouncePadSquash_;

    // --- progress -------------------------------------------------------------
    int coinsCollected_ = 0;
    int enemiesDefeated_ = 0;
    int lives_ = 0;
    float invulnerableTimer_ = 0.0f;
    int falls_ = 0;
    bool reachedGoal_ = false;
    float goalHoldTimer_ = 0.0f;

    float elapsedTime_ = 0.0f;
    glm::vec3 cameraPosition_{0.0f};
    float uiScale_ = 1.0f;
};
