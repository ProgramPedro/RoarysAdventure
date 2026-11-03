#pragma once
//=============================================================================
// TunnelScene - Floor 2, "The Shifting Tunnel".
//
// You auto-run down a square tunnel. Run off the side of the surface you are
// on and you carry on across the neighbouring wall, which becomes your new
// floor; the world holds still and your sense of down rotates.
//
// CONTROLS
//   A / D, arrows   strafe across the face you are on
//   Space           jump
//   R               restart the run
//   Esc             back to the tower
//   C               DEBUG: mark the floor complete
//
// HOW THE ORIENTATION IS STORED
//   Not as a rotation. The player is on one of FOUR faces, held as an integer
//   0-3, and gravity points at whichever face that is. Only the camera's roll
//   angle is interpolated, purely for looks. Integrating a free rotation here
//   is the known way to accumulate drift; four discrete states cannot drift.
//
// The layout lives in TunnelLevel.h and the tuning in cfg::tunnel.
//=============================================================================

#include "core/Scene.h"

#include <glm/vec3.hpp>

class TunnelScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return "TunnelScene"; }

    // --- the four faces -------------------------------------------------------
    // Face 0 is the floor and face + 1 is always whichever face you reach by
    // strafing RIGHT on screen. Public because they are pure geometry and the
    // corner continuity between them is worth testing directly.
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

    // --- player ---------------------------------------------------------------
    int   face_ = 0;
    float lateral_ = 0.0f;    // position across the current face
    float height_ = 0.0f;     // distance out from the face surface
    float distance_ = 0.0f;   // how far down the tunnel
    float lateralVelocity_ = 0.0f;
    float upVelocity_ = 0.0f;
    float forwardSpeed_ = 0.0f;
    bool grounded_ = false;
    float coyoteTimer_ = 0.0f;

    // --- input captured this frame -------------------------------------------
    float strafeInput_ = 0.0f;
    bool jumpQueued_ = false;

    // --- presentation ---------------------------------------------------------
    float rollDegrees_ = 0.0f;   // follows face_ * 90, smoothly
    float elapsedTime_ = 0.0f;
    float uiScale_ = 1.0f;

    // --- progress -------------------------------------------------------------
    int wipeouts_ = 0;
    bool finished_ = false;
    float finishHoldTimer_ = 0.0f;
};
