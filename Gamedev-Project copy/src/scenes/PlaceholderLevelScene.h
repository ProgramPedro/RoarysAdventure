#pragma once
//=============================================================================
// PlaceholderLevelScene - a stand-in for a real floor.
//
// It shows which floor you walked into, and gives you two keys:
//   Esc - go back to the level select screen
//   C   - mark this floor COMPLETE (which unlocks the next one) and go back
//
// The C key is a debug shortcut so the unlock chain can be tested end to end
// before any actual gameplay exists. When a real level replaces this scene, the
// real win condition should make the same markCompleted() call this one does.
//=============================================================================

#include "core/Scene.h"

class PlaceholderLevelScene : public Scene {
public:
    // floorIndex is 0-based: 0 is floor 1, 3 is the top floor.
    explicit PlaceholderLevelScene(int floorIndex);

    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;

    const char* name() const override { return name_; }

private:
    void returnToMenu();

    int   floorIndex_ = 0;
    float elapsedTime_ = 0.0f;
    const char* name_ = "PlaceholderLevelScene";
};
