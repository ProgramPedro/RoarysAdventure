#pragma once
//=============================================================================
// Scene - the interface every screen in the game implements.
//
// THE LIFECYCLE, in the order it happens:
//
//   attach(ctx)   SceneManager hands the scene its shared context. Called once,
//                 before init(). Scenes do not override this.
//   init()        Set up whatever this scene needs. Return false to abort.
//   ... then once per frame, for as long as the scene is current:
//   handleInput() React to keys and mouse. Read state, do not draw.
//   update(dt)    Advance animation and logic by dt seconds.
//   render()      Draw. Do not change game state here.
//   ...
//   cleanup()     Release anything init() created. Called once, just before
//                 the scene is destroyed.
//
// SWITCHING SCENES: never delete a scene from inside itself. Call
//   ctx().manager.requestScene(SceneId::Whatever);
// and the SceneManager performs the switch after the current frame has
// finished drawing. See SceneManager.h for why that matters.
//=============================================================================

struct SceneContext;
class Input;

class Scene {
public:
    virtual ~Scene() = default;

    // Called by SceneManager before init(). Not meant to be overridden.
    void attach(SceneContext* context) { context_ = context; }

    virtual bool init() = 0;
    virtual void handleInput(const Input& input) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;

    // Optional: only override if the scene allocated something in init().
    virtual void cleanup() {}

    // Used in log messages so it is obvious which scene is running.
    virtual const char* name() const = 0;

protected:
    // Shared services: the window size, the renderer, GameProgress and the
    // SceneManager. Valid from init() onward.
    SceneContext& ctx() { return *context_; }
    const SceneContext& ctx() const { return *context_; }

private:
    SceneContext* context_ = nullptr;
};
