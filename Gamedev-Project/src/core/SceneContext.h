#pragma once

// The bundle of shared services every scene is handed: the window, the live
// framebuffer size, the renderer, the save state and the scene manager. Passing
// this to each scene is what keeps the game free of globals. Everything it
// references outlives every scene, so it is valid from init() through cleanup().

struct GLFWwindow;
class Renderer;
class GameProgress;
class SceneManager;

struct SceneContext {
    GLFWwindow*   window = nullptr;

    int framebufferWidth  = 1;
    int framebufferHeight = 1;

    Renderer&     renderer;
    GameProgress& progress;
    SceneManager& manager;

    SceneContext(Renderer& rendererRef, GameProgress& progressRef,
                 SceneManager& managerRef)
        : renderer(rendererRef), progress(progressRef), manager(managerRef) {}
};
