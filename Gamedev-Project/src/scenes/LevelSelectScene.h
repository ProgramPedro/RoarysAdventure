#pragma once

// The title screen and the level menu in one: the tower is the menu, four stacked
// rooms drawn as an open-fronted cutaway and each themed after the game that
// floor is based on, with Roary on a lift that slides to the selected floor.
// W/S or the mouse change the selection, Enter or a click enters an unlocked
// floor, and Esc quits.

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
    void moveSelection(int delta);
    void tryEnterSelectedFloor();

    void refreshFloorScreenRects();

    int floorUnderMouse(const glm::vec2& mousePosition) const;

    void drawTower();
    void drawFloorShell(int floorIndex);
    void drawFloorProps(int floorIndex);
    void drawSelectionFrame(int floorIndex);
    void drawLockIcon(int floorIndex);
    void drawRoary();
    void drawUi();

    void drawPlatformerProps(float centerY, bool locked);
    void drawTunnelProps(float centerY, bool locked);
    void drawStealthProps(float centerY, bool locked);
    void drawBossProps(float centerY, bool locked);

    void drawOwl(const glm::vec3& feetPosition, bool locked);
    void drawIbis(const glm::vec3& feetPosition, bool locked);
    void drawPawlinaInCage(const glm::vec3& feetPosition, bool locked);

    float floorPickHeight(int floorIndex) const;

    glm::vec3 shade(const glm::vec3& color, bool locked) const;

    float px(float designPixels) const;

    int   selectedFloor_ = 0;
    int   hoveredFloor_  = -1;
    float elapsedTime_   = 0.0f;

    float roaryY_ = 0.0f;

    float lockedFlashTimer_ = 0.0f;

    glm::vec2 lastMousePosition_{0.0f, 0.0f};

    std::array<glm::vec4, cfg::kFloorCount> floorScreenRects_{};

    float uiScale_ = 1.0f;
};
