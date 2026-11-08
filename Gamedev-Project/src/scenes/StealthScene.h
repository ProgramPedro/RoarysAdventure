#pragma once
// StealthScene - Floor 3, "Silent Paws"
// Sneak past the guard, collect the four paws, and leave through the door.
// Rocks can be thrown to distract the guard. Tuning values are in Config.h (kF3).

#include "core/Camera.h"
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
    bool guardCanSeePlayer() const;
    bool guardIsDistracted() const { return guardAlertTimer_ > 0.0f; }

    void updateSuspicion(float deltaTime);
    void getCaught();

    void collectNearbyItems();
    int  collectedCount() const;
    bool doorIsOpen() const;
    void checkDoor();

    void updateAim(const Input& input);
    void throwRock();
    void updateRock(float deltaTime);
    void raiseNoise(const glm::vec3& position);

    Camera makeCamera() const;
    bool screenToFloor(const glm::vec2& screenPos, glm::vec3& outFloor) const;

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
    void drawSuspicionMeter();
    void drawUi();

    float px(float designPixels) const;

    // Roary
    glm::vec3 playerPos_{0.0f};
    glm::vec2 moveDir_{0.0f, 0.0f};
    float     facingDeg_ = 180.0f;
    bool      sneaking_  = false;

    // Guard
    glm::vec3 guardPos_{0.0f};
    float     guardFacingDeg_  = 0.0f;
    int       patrolIndex_     = 0;
    float     guardPauseTimer_ = 0.0f;
    float     guardAlertTimer_ = 0.0f;
    glm::vec3 guardAlertTarget_{0.0f};
    bool      playerSeen_      = false;

    // Suspicion
    float suspicion_        = 0.0f;
    float sightLostTimer_   = 0.0f;
    float caughtFlashTimer_ = 0.0f;

    // Paws and door
    std::array<bool, cfg::kF3ItemCount> itemCollected_{};
    float lockedFlashTimer_ = 0.0f;

    // Rocks
    int       rocksLeft_    = 0;
    bool      aiming_       = false;
    bool      aimValid_     = false;
    bool      aimBlocked_   = false;
    glm::vec3 aimImpact_{0.0f};
    bool      rockInFlight_ = false;
    glm::vec3 rockFrom_{0.0f};
    glm::vec3 rockTo_{0.0f};
    float     rockElapsed_  = 0.0f;
    float     rockDuration_ = 0.0f;
    glm::vec3 noisePos_{0.0f};
    float     noiseTimer_   = 0.0f;

    float uiScale_ = 1.0f;
};
