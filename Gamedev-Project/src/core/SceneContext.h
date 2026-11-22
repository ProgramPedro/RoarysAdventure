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

#include <glm/vec3.hpp>

struct GLFWwindow;
class Renderer;
class GameProgress;
class SceneManager;

//-----------------------------------------------------------------------------
// Where to drop the player when a level starts, if the command line said so.
//
// This exists for PLAYTESTING. Watching someone replay the first two minutes
// to get to the part you actually want them to try wastes their session, so
// --at and --dist let you start them at a fork or partway down the tunnel.
// Nothing in the game sets these; without the flags they stay off.
//-----------------------------------------------------------------------------
struct DebugStart {
    bool      overridePosition = false;   // --at x,y,z (Floor 1)
    glm::vec3 position{0.0f};
    bool      overrideDistance = false;   // --dist metres (Floor 2)
    float     distance = 0.0f;
};

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

    DebugStart    debugStart{};  // set from the command line, off by default

    SceneContext(Renderer& rendererRef, GameProgress& progressRef,
                 SceneManager& managerRef)
        : renderer(rendererRef), progress(progressRef), manager(managerRef) {}
};
