#pragma once
//=============================================================================
// LevelSelectScene - the title screen and the level menu in one.
//
// The tower IS the menu: four stacked rooms drawn as an open-fronted cutaway,
// each themed after the game that floor is based on. Roary stands on a lift to
// the left of the tower and slides up and down to whichever floor is selected.
//
// CONTROLS
//   W / Up, S / Down   change the selected floor
//   Enter / Space      enter the selected floor (if it is unlocked)
//   Mouse              hover to select, click to enter
//   Esc                quit the game
//   U                  DEBUG: unlock every floor
//=============================================================================

#include "core/Config.h"
#include "core/Scene.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <array>

class LevelSelectScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return "LevelSelectScene"; }

private:
    // --- selection -----------------------------------------------------------
    void moveSelection(int delta);
    void tryEnterSelectedFloor();

    // Recomputes where each floor lands on screen, for mouse hit-testing and
    // for placing the little floor labels.
    void refreshFloorScreenRects();

    // Which floor the mouse is over, or -1 for none.
    int floorUnderMouse(const glm::vec2& mousePosition) const;

    // --- drawing -------------------------------------------------------------
    void drawTower();
    void drawFloorShell(int floorIndex);
    void drawFloorProps(int floorIndex);
    void drawSelectionFrame(int floorIndex);
    void drawLockIcon(int floorIndex);
    void drawRoary();
    void drawUi();

    // Per-floor themed contents.
    void drawPlatformerProps(float centerY, bool locked);  // floor 1
    void drawTunnelProps(float centerY, bool locked);      // floor 2
    void drawStealthProps(float centerY, bool locked);     // floor 3
    void drawBossProps(float centerY, bool locked);        // floor 4

    // Small helpers for the boss floor.
    void drawOwl(const glm::vec3& feetPosition, bool locked);
    void drawIbis(const glm::vec3& feetPosition, bool locked);
    void drawBull(const glm::vec3& feetPosition, bool locked);
    void drawPawlinaInCage(const glm::vec3& feetPosition, bool locked);

    // How tall a floor counts as on screen. The open top floor is shorter
    // than its room, because most of that room is sky.
    float floorPickHeight(int floorIndex) const;

    // Fades a color toward grey when a floor is locked.
    glm::vec3 shade(const glm::vec3& color, bool locked) const;

    // Converts a design-pixel measurement into real framebuffer pixels.
    float px(float designPixels) const;

    // --- state ---------------------------------------------------------------
    int   selectedFloor_ = 0;
    int   hoveredFloor_  = -1;
    float elapsedTime_   = 0.0f;

    // Roary's lift slides smoothly instead of snapping between floors.
    float roaryY_ = 0.0f;

    // Counts down after the player tries to enter a locked floor, so the
    // LOCKED message can flash instead of the press doing nothing visible.
    float lockedFlashTimer_ = 0.0f;

    // Last frame's cursor position, so hover only takes over the selection
    // once the mouse has actually moved.
    glm::vec2 lastMousePosition_{0.0f, 0.0f};

    // Screen-space bounding box of each floor: (minX, minY, maxX, maxY).
    std::array<glm::vec4, cfg::kFloorCount> floorScreenRects_{};

    float uiScale_ = 1.0f;
};
