#include "core/SceneManager.h"

#include "core/Scene.h"
#include "core/SceneContext.h"

// The concrete scenes are included HERE and nowhere else, which keeps the rest
// of the engine from depending on any particular level.
#include "scenes/LevelSelectScene.h"
#include "scenes/PlaceholderLevelScene.h"

#include <iostream>

void SceneManager::requestScene(SceneId id) {
    if (id == SceneId::None) {
        return;
    }

    // If two requests land in the same frame, the last one wins. That is the
    // least surprising behavior, and in practice it does not happen.
    pendingId_ = id;
    switchPending_ = true;
}

std::unique_ptr<Scene> SceneManager::createScene(SceneId id) {
    switch (id) {
        case SceneId::LevelSelect:
            return std::make_unique<LevelSelectScene>();

        // Each floor currently gets the same placeholder scene, told which
        // floor it is standing in for. When a real level is written, swap the
        // line for that floor - for example:
        //     case SceneId::Floor1: return std::make_unique<PlatformerScene>();
        case SceneId::Floor1: return std::make_unique<PlaceholderLevelScene>(0);
        case SceneId::Floor2: return std::make_unique<PlaceholderLevelScene>(1);
        case SceneId::Floor3: return std::make_unique<PlaceholderLevelScene>(2);
        case SceneId::Floor4: return std::make_unique<PlaceholderLevelScene>(3);

        case SceneId::None:
        default:
            return nullptr;
    }
}

void SceneManager::applyPendingSwitch(SceneContext& context) {
    if (!switchPending_) {
        return;
    }
    switchPending_ = false;

    const SceneId targetId = pendingId_;
    pendingId_ = SceneId::None;

    // Let the outgoing scene release anything it owns before it disappears.
    if (current_ != nullptr) {
        std::cout << "[SceneManager] leaving " << current_->name() << '\n';
        current_->cleanup();
        current_.reset();
    }

    std::unique_ptr<Scene> nextScene = createScene(targetId);
    if (nextScene == nullptr) {
        std::cerr << "[SceneManager] no scene is registered for that SceneId. "
                     "Did you add a case to createScene()?\n";
        requestQuit();
        return;
    }

    // Give the scene its shared context BEFORE init(), so init() can already
    // use the renderer and read GameProgress.
    nextScene->attach(&context);

    if (!nextScene->init()) {
        std::cerr << "[SceneManager] " << nextScene->name()
                  << " failed to initialize.\n";
        requestQuit();
        return;
    }

    std::cout << "[SceneManager] entering " << nextScene->name() << '\n';
    current_ = std::move(nextScene);
}

void SceneManager::shutdown() {
    if (current_ != nullptr) {
        current_->cleanup();
        current_.reset();
    }
}
