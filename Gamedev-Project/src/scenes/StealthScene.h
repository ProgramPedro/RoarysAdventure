#pragma once
// StealthScene - Floor 3, "Silent Paws"
// Sneak past the guard, collect the four paws, and leave through the door.
// Rocks can be thrown to distract the guard. Tuning values are in Config.h (kF3).

#include "core/Config.h"
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
    static const int kFloorIndex = 2;

    void returnToMenu();

    void movePlayer(float deltaTime);
    glm::vec3 clampToRoom(const glm::vec3& position, float radius) const;
    void resolveInnerWalls(glm::vec3& position, int axis, float delta,
                           float radius) const;

    static bool pointInsideWall(const glm::vec3& point);
    static glm::vec3 walkUntilWall(const glm::vec3& from, const glm::vec3& to,
                                   bool& outBlocked);

    void updateGuard(float deltaTime);
    void resetGuard();
    float visionRange() const;
    bool guardCanSeePlayer() const;
    bool guardIsDistracted() const { return guardAlertTimer_ > 0.0f; }

    void getCaught();

    void collectNearbyItems();
    int  collectedCount() const;
    bool doorIsOpen() const;
    void checkDoor();

    void throwRock();

    void drawRoom();
    void drawRoary();
    void drawGuard();
    void drawVisionCone();
    void drawItems();
    void drawDoor();
    void drawUi();

    float px(float designPixels) const;

    // Roary
    glm::vec3 playerPos_{0.0f};
    glm::vec2 moveDir_{0.0f, 0.0f};
    float     facingDeg_        = 180.0f;
    bool      sneaking_         = false;
    float     caughtFlashTimer_ = 0.0f;

    // Guard
    glm::vec3 guardPos_{0.0f};
    float     guardFacingDeg_         = 0.0f;
    int       patrolIndex_            = 0;
    float     guardPauseTimer_        = 0.0f;
    float     guardAlertTimer_        = 0.0f;
    glm::vec3 guardAlertTarget_{0.0f};
    float     guardFacingBeforeAlert_ = 0.0f;

    // Paws
    std::array<bool, cfg::kF3ItemCount> itemCollected_{};

    // Rocks
    int rocksLeft_ = 0;

    float uiScale_ = 1.0f;
};
