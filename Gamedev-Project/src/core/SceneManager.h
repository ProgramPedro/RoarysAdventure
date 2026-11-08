#pragma once

// Owns the one scene that is currently running and switches between scenes
// safely. requestScene() only records which scene is wanted next, and the switch
// is applied after the frame has been drawn, so a scene can ask to leave from
// inside its own input, update or render code without being destroyed mid-frame.

#include "core/SceneId.h"

#include <memory>

class Scene;
struct SceneContext;

class SceneManager {
public:
    void requestScene(SceneId id);

    void requestQuit() { quitRequested_ = true; }
    bool quitRequested() const { return quitRequested_; }

    Scene* currentScene() const { return current_.get(); }
    bool hasScene() const { return current_ != nullptr; }

    void applyPendingSwitch(SceneContext& context);

    void shutdown();

private:
    static std::unique_ptr<Scene> createScene(SceneId id);

    std::unique_ptr<Scene> current_;
    SceneId pendingId_ = SceneId::None;
    bool switchPending_ = false;
    bool quitRequested_ = false;
};
