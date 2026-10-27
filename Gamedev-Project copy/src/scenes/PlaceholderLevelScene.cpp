#include "scenes/PlaceholderLevelScene.h"

#include "core/Camera.h"
#include "core/Config.h"
#include "core/GameProgress.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/SceneContext.h"
#include "core/SceneManager.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace {

// The accent color for each floor, so the placeholder already hints at the
// theme of the level that will eventually live here.
glm::vec3 themeColorFor(int floorIndex) {
    switch (floorIndex) {
        case 0:  return cfg::kF1BlockWarm;
        case 1:  return cfg::kF2RimGlow;
        case 2:  return cfg::kF3Wall;
        case 3:  return cfg::kF4Wall;
        default: return cfg::kTowerStone;
    }
}

} // namespace

PlaceholderLevelScene::PlaceholderLevelScene(int floorIndex)
    : floorIndex_(std::clamp(floorIndex, 0, cfg::kFloorCount - 1)) {}

bool PlaceholderLevelScene::init() {
    elapsedTime_ = 0.0f;
    return true;
}

void PlaceholderLevelScene::returnToMenu() {
    // Note we only REQUEST the switch. This scene keeps running for the rest of
    // the frame and SceneManager swaps it out afterwards.
    ctx().manager.requestScene(SceneId::LevelSelect);
}

void PlaceholderLevelScene::handleInput(const Input& input) {
    // Esc: back out without finishing the floor.
    if (input.wasPressed(GLFW_KEY_ESCAPE)) {
        returnToMenu();
        return;
    }

    // C: debug "I beat this floor". This is the exact call a real level should
    // make when the player actually wins.
    if (input.wasPressed(GLFW_KEY_C)) {
        ctx().progress.markCompleted(floorIndex_);
        returnToMenu();
    }
}

void PlaceholderLevelScene::update(float deltaTime) {
    elapsedTime_ += deltaTime;
}

void PlaceholderLevelScene::render() {
    Renderer& renderer = ctx().renderer;
    const int width = ctx().framebufferWidth;
    const int height = ctx().framebufferHeight;

    const glm::vec3 theme = themeColorFor(floorIndex_);

    // Tint the usual FIU-blue backdrop toward this floor's theme color, so each
    // placeholder floor is instantly distinguishable from the others.
    renderer.drawBackground(
        glm::mix(cfg::kBgTop, theme, 0.45f),
        glm::mix(cfg::kBgBottom, theme, 0.20f),
        cfg::kBgGlow,
        glm::vec2(0.0f, 0.0f),
        cfg::kBgGlowRadius,
        0.28f);

    // A single slowly spinning box, just so it is obvious the scene is live and
    // that delta time is reaching update().
    const glm::mat4 spin = makeTransform(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.6f, 1.6f, 1.6f),
        glm::vec3(18.0f, elapsedTime_ * 42.0f, 0.0f));

    Camera camera(glm::vec3(0.0f, 1.2f, 6.5f), glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.setCamera(camera.view(), camera.perspective(width, height));
    renderer.drawBox(spin, theme);

    // ---- text overlay -------------------------------------------------------
    // Positions and sizes are written in design pixels against an 800-pixel
    // tall window, then scaled, so the text is the same apparent size on a
    // Retina display as on a normal one. The menu does exactly the same thing.
    const float uiScale = static_cast<float>(height) / cfg::kUiReferenceHeight;
    const auto px = [uiScale](float designPixels) { return designPixels * uiScale; };

    TextRenderer& text = renderer.text();
    text.begin(width, height);

    const float centerX = static_cast<float>(width) * 0.5f;
    const float bottom = static_cast<float>(height);
    const cfg::FloorInfo& info = cfg::kFloorInfo[floorIndex_];

    text.drawCentered(info.name, centerX, px(56.0f),
                      px(cfg::kFloorNameScale), cfg::kTextPrimary);

    text.drawCentered(info.description, centerX, px(104.0f),
                      px(cfg::kDescriptionScale), cfg::kTextDim);

    text.drawCentered("PLACEHOLDER LEVEL - NO GAMEPLAY YET",
                      centerX, bottom - px(132.0f),
                      px(cfg::kHintScale), cfg::kFiuGold);

    const bool alreadyDone = ctx().progress.isCompleted(floorIndex_);
    const std::string completeHint = alreadyDone
        ? "C - MARK COMPLETE (ALREADY COMPLETE)"
        : "C - MARK COMPLETE AND UNLOCK THE NEXT FLOOR";

    text.drawCentered(completeHint, centerX, bottom - px(94.0f),
                      px(cfg::kHintScale), cfg::kTextPrimary);

    text.drawCentered("ESC - BACK TO THE TOWER", centerX, bottom - px(56.0f),
                      px(cfg::kHintScale), cfg::kTextDim);

    text.end();
}
