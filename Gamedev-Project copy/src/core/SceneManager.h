#pragma once
//=============================================================================
// SceneManager - owns the one scene that is currently running, and switches
// between scenes safely.
//
// THE PROBLEM THIS SOLVES: if a scene deleted itself the moment the player
// pressed a key, the rest of that frame (the update and render calls still to
// come) would be running on a destroyed object. Classic crash.
//
// THE FIX: requestScene() only writes down which scene we want next. Nothing
// is created or destroyed yet, so the current frame finishes normally.
// main() then calls applyPendingSwitch() AFTER the frame has been drawn and
// presented, and that is where the old scene is cleaned up and the new one is
// initialized. A scene can therefore safely call requestScene() from inside its
// own handleInput(), update() or render() without any special care.
//=============================================================================

#include "core/SceneId.h"

#include <memory>

class Scene;
struct SceneContext;

class SceneManager {
public:
    // Ask to switch scenes. Safe to call from inside a running scene. Takes
    // effect at the end of the current frame.
    void requestScene(SceneId id);

    // Ask to close the game. main() checks this to leave the loop.
    void requestQuit() { quitRequested_ = true; }
    bool quitRequested() const { return quitRequested_; }

    // The scene running right now, or nullptr before the first switch applies.
    Scene* currentScene() const { return current_.get(); }
    bool hasScene() const { return current_ != nullptr; }

    // Performs any pending switch. Call this once per frame, AFTER the current
    // frame has been rendered and the buffers swapped.
    //
    // Order of operations: old scene's cleanup() runs, old scene is destroyed,
    // new scene is constructed, attached to the context, then init() runs.
    void applyPendingSwitch(SceneContext& context);

    // Tears down whatever scene is current. Called by main() on shutdown.
    void shutdown();

private:
    // The ONLY place that knows about concrete scene classes. Add a case here
    // when you add a SceneId.
    static std::unique_ptr<Scene> createScene(SceneId id);

    std::unique_ptr<Scene> current_;
    SceneId pendingId_ = SceneId::None;
    bool switchPending_ = false;
    bool quitRequested_ = false;
};
