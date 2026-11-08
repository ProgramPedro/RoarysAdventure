#pragma once

// The interface every screen in the game implements: init, then handleInput,
// update and render once per frame, and cleanup at the end. A scene never deletes
// itself - it calls requestScene() on the SceneManager, which performs the switch
// after the frame has finished drawing.

struct SceneContext;
class Input;

class Scene {
public:
    virtual ~Scene() = default;

    void attach(SceneContext* context) { context_ = context; }

    virtual bool init() = 0;
    virtual void handleInput(const Input& input) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;

    virtual void cleanup() {}

    virtual const char* name() const = 0;

protected:
    SceneContext& ctx() { return *context_; }
    const SceneContext& ctx() const { return *context_; }

private:
    SceneContext* context_ = nullptr;
};
