// Implements the deferred scene switch: a request is recorded, then applied once
// the frame is done by cleaning up the old scene and initializing the new one. If
// two requests land in the same frame the last one wins. This is the only file
// that knows the concrete scene classes.

#include "core/SceneManager.h"

#include "core/Scene.h"
#include "core/SceneContext.h"

#include "scenes/LevelSelectScene.h"
#include "scenes/PlatformerScene.h"
#include "scenes/TunnelScene.h"
#include "scenes/StealthScene.h"
#include "scenes/BossScene.h"

#include <iostream>

void SceneManager::requestScene(SceneId id) {
    if (id == SceneId::None) {
        return;
    }

    pendingId_ = id;
    switchPending_ = true;
}

std::unique_ptr<Scene> SceneManager::createScene(SceneId id) {
    switch (id) {
        case SceneId::LevelSelect:
            return std::make_unique<LevelSelectScene>();

        case SceneId::Floor1: return std::make_unique<PlatformerScene>();
        case SceneId::Floor2: return std::make_unique<TunnelScene>();
        case SceneId::Floor3: return std::make_unique<StealthScene>();
        case SceneId::Floor4: return std::make_unique<BossScene>();

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
