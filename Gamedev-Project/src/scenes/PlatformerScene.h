#pragma once

// Floor 1, a 3D platformer: run and jump along a course of platforms to the goal,
// collecting coins. Enemies can be stomped except the spiked one, item blocks
// drop a coin when punched from below, bounce pads throw you higher than a jump
// can reach, and three lives with knockback are the cost of failing. The layout
// lives in PlatformerLevel.h and the feel is tuned from cfg::plat.

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
    struct MovingPlatform {
        glm::vec3 from{0.0f};
        glm::vec3 to{0.0f};
        glm::vec3 size{1.0f};
        float speed = 1.0f;
        float travel = 0.0f;
        int direction = 1;
        glm::vec3 center{0.0f};
        glm::vec3 lastDelta{0.0f};
    };

    struct Enemy {
        level1::EnemyKind kind = level1::EnemyKind::Walker;
        glm::vec3 position{0.0f};
        float minX = 0.0f;
        float maxX = 0.0f;
        float speed = 1.0f;
        int direction = 1;
        bool alive = true;
        float squashTimer = 0.0f;
    };

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

    bool findGroundShadow(float& outY) const;

    Aabb player_{};
    glm::vec3 velocity_{0.0f};
    bool grounded_ = false;
    bool jumping_ = false;
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    float facingYaw_ = 0.0f;
    glm::vec3 lastSafePosition_{0.0f};

    glm::vec2 moveInput_{0.0f};
    bool jumpHeld_ = false;

    std::vector<Aabb> staticSolids_;
    std::vector<MovingPlatform> movingPlatforms_;
    std::vector<Aabb> solids_;
    std::vector<bool> coinTaken_;
    std::vector<bool> itemBlockUsed_;
    std::vector<Enemy> enemies_;
    std::vector<PoppedCoin> poppedCoins_;
    std::vector<float> bouncePadSquash_;

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
