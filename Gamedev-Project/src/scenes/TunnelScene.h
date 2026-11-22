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
//
// THE TILES
//   Besides plain tiles and holes there are crates to dodge, crumbling tiles
//   that drop away a moment after you touch them, springs that launch you
//   over gaps nothing else can cross, speed tiles and ice. What each one does
//   is tuned in cfg::tunnel; where they are is TunnelLevel.h.
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
#include <vector>

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

    // The glyph the player is standing on (or over), from TunnelLevel.h.
    char tileHere() const;

    // Where a tile's crumble state lives in crumble_. Returns -1 if the
    // position is off the end of the level.
    int crumbleIndex(int segment, int face, int lane) const;

    // Has this crumbling tile already fallen away this run?
    bool crumbledAway(int segment, int face, int lane) const;

    // Is there something solid to stand on here, right now? Differs from
    // level2::hasTile only in that a crumbled tile no longer counts.
    bool solidAt(int segment, int face, int lane) const;
    bool tileUnderFoot() const;

    void restartLevel();
    void updateRun(float deltaTime);
    void updateRoll(float deltaTime);
    void applyTileEffects(float deltaTime);

    void drawTunnel();
    void drawHud();

    // --- player ---------------------------------------------------------------
    int   face_ = 0;
    float lateral_ = 0.0f;    // position across the current face
    float height_ = 0.0f;     // distance out from the face surface
    float distance_ = 0.0f;   // how far down the tunnel
    float startDistance_ = 0.0f;  // where a restart puts you (--dist, else 0)
    float lateralVelocity_ = 0.0f;
    float upVelocity_ = 0.0f;

    // Forward speed is two parts added together: a ramp that only ever creeps
    // upward over the run, and a boost from a speed tile that bleeds away.
    // Keeping them separate means a boost cannot permanently change the pace.
    float rampSpeed_ = 0.0f;
    float boost_ = 0.0f;
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

    // --- the state of the level itself ----------------------------------------
    // One entry per (segment, face, lane). Below zero means "nobody has stood
    // on it"; otherwise it counts the seconds since they did, and past
    // cfg::tunnel::kCrumbleDelay the tile is gone for the rest of the run.
    std::vector<float> crumble_;

    int checkpointSegment_ = 0;   // where a wipeout sends you, if enabled

    // --- progress -------------------------------------------------------------
    int wipeouts_ = 0;
    bool finished_ = false;
    float finishHoldTimer_ = 0.0f;
};
