#pragma once

// Floor 2, an auto-run down a square tunnel where running off the edge of the
// surface you are on carries you onto the neighbouring wall, which becomes your
// new floor. Orientation is held as one of four face indices rather than a free
// rotation, so it cannot drift; only the camera's roll is interpolated, purely
// for looks. The layout lives in TunnelLevel.h and the tuning in cfg::tunnel.

#include "core/Scene.h"

#include <glm/vec3.hpp>

class TunnelScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return "TunnelScene"; }

    static glm::vec3 faceDown(int face);
    static glm::vec3 faceUp(int face);
    static glm::vec3 faceRight(int face);

private:
    glm::vec3 worldPosition() const;
    int currentSegment() const;
    int currentLane() const;
    bool tileUnderFoot() const;

    void restartLevel();
    void updateRun(float deltaTime);
    void updateRoll(float deltaTime);

    void drawTunnel();
    void drawHud();

    int   face_ = 0;
    float lateral_ = 0.0f;
    float height_ = 0.0f;
    float distance_ = 0.0f;
    float lateralVelocity_ = 0.0f;
    float upVelocity_ = 0.0f;
    float forwardSpeed_ = 0.0f;
    bool grounded_ = false;
    float coyoteTimer_ = 0.0f;

    float strafeInput_ = 0.0f;
    bool jumpQueued_ = false;

    float rollDegrees_ = 0.0f;
    float elapsedTime_ = 0.0f;
    float uiScale_ = 1.0f;

    int wipeouts_ = 0;
    bool finished_ = false;
    float finishHoldTimer_ = 0.0f;
};
