#pragma once
//=============================================================================
// SceneContext - the bundle of shared things every scene is allowed to touch.
//
// One of these is created in main() and passed to each scene, so scenes get at
// the renderer, the window size, the save state and the scene manager without
// any globals.
//
// The references here are all to objects that outlive every scene, so they are
// always valid from init() through cleanup().
//=============================================================================

struct GLFWwindow;
class Renderer;
class GameProgress;
class SceneManager;

struct SceneContext {
    GLFWwindow*   window = nullptr;

    // Live framebuffer size in pixels, refreshed by main() every frame. Use
    // these for UI layout rather than the config constants, so the game keeps
    // laying out correctly when the window is resized.
    int framebufferWidth  = 1;
    int framebufferHeight = 1;

    Renderer&     renderer;   // shared shader, meshes and text drawing
    GameProgress& progress;   // which floors are unlocked / completed
    SceneManager& manager;    // call requestScene() on this to change screens

    SceneContext(Renderer& rendererRef, GameProgress& progressRef,
                 SceneManager& managerRef)
        : renderer(rendererRef), progress(progressRef), manager(managerRef) {}
};
