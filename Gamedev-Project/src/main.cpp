//=============================================================================
// Roary's Adventure: Save Pawlina!
// CAP 4052 group project
//
// main() is deliberately thin. It owns the window, the OpenGL context and the
// long-lived objects (Renderer, GameProgress, SceneManager), then hands every
// frame to whichever Scene is currently running. All the actual game logic and
// drawing lives in the scenes under src/scenes/.
//
// THE FRAME, in order:
//   1. poll window events
//   2. work out delta time
//   3. snapshot keyboard and mouse into Input (this is what makes
//      wasPressed() edge detection work)
//   4. scene->handleInput()  - react to the player
//   5. scene->update(dt)     - advance the simulation
//   6. scene->render()       - draw
//   7. swap buffers
//   8. manager.applyPendingSwitch() - change scenes, if one was requested
//
// Step 8 happening LAST is the important part: a scene can ask to switch at any
// point during steps 4-6 and still finish its own frame safely.
//=============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/Config.h"
#include "core/GameProgress.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/Scene.h"
#include "core/SceneContext.h"
#include "core/SceneManager.h"
#include "core/Screenshot.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

namespace {

// Keeps the GL viewport in step with the window whenever it is resized. The
// scenes read the live size out of SceneContext every frame, so there is
// nothing else to update here.
void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, std::max(width, 1), std::max(height, 1));
}

//-----------------------------------------------------------------------------
// Command line options. All of them are development conveniences; the game
// needs no arguments to run normally.
//
//   --screenshot <file>  render a few frames, save a PPM image, then exit
//   --frames <n>         how many frames to render first (default 3), so any
//                        animation has settled before the image is taken
//   --unlock-all         start with every floor unlocked
//   --floor <1-4>        open straight into that floor instead of the menu
//-----------------------------------------------------------------------------
struct Options {
    std::string screenshotPath;
    int  framesBeforeScreenshot = 3;
    bool unlockAll = false;
    int  startFloor = 0;  // 0 means "start on the level select screen"
};

Options parseCommandLine(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (std::strcmp(arg, "--screenshot") == 0 && i + 1 < argc) {
            options.screenshotPath = argv[++i];
        } else if (std::strcmp(arg, "--frames") == 0 && i + 1 < argc) {
            options.framesBeforeScreenshot = std::max(1, std::atoi(argv[++i]));
        } else if (std::strcmp(arg, "--unlock-all") == 0) {
            options.unlockAll = true;
        } else if (std::strcmp(arg, "--floor") == 0 && i + 1 < argc) {
            options.startFloor = std::clamp(std::atoi(argv[++i]), 0, cfg::kFloorCount);
        } else {
            std::cerr << "Unknown option: " << arg << '\n';
        }
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    const Options options = parseCommandLine(argc, argv);
    //-------------------------------------------------------------------------
    // GLFW + OpenGL 3.3 core context
    //-------------------------------------------------------------------------
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW initialization failed.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // macOS only gives you a 3.3+ core context if you ask for forward
    // compatibility. This hint is harmless on Windows and Linux.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        cfg::kWindowWidth, cfg::kWindowHeight, cfg::kWindowTitle, nullptr, nullptr);

    if (window == nullptr) {
        std::cerr << "Could not create a GLFW window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // VSync on

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "GLAD could not load OpenGL functions.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';

    //-------------------------------------------------------------------------
    // Long-lived game objects. These outlive every scene, which is why
    // SceneContext can safely hold references to them.
    //-------------------------------------------------------------------------
    Renderer renderer;
    if (!renderer.init()) {
        std::cerr << "Renderer initialization failed.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GameProgress progress;
    SceneManager manager;
    Input input;

    if (options.unlockAll) {
        progress.unlockAll();
    }

    SceneContext context(renderer, progress, manager);
    context.window = window;
    glfwGetFramebufferSize(window, &context.framebufferWidth,
                                   &context.framebufferHeight);

    // The game opens on the level-select screen (unless --floor sent us
    // straight into a level). applyPendingSwitch() is what actually builds the
    // scene, so call it once before the loop starts.
    manager.requestScene(options.startFloor > 0
                             ? sceneIdForFloor(options.startFloor - 1)
                             : SceneId::LevelSelect);
    manager.applyPendingSwitch(context);

    //-------------------------------------------------------------------------
    // Main loop
    //-------------------------------------------------------------------------
    double previousTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window) && !manager.quitRequested()) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        // Clamp delta time. Without this, dragging the window or hitting a
        // breakpoint produces one enormous frame that teleports everything.
        deltaTime = std::clamp(deltaTime, 0.0f, 0.1f);

        // Must happen after glfwPollEvents so the state is current.
        input.update(window);

        // Refresh the size every frame so the UI follows window resizes.
        glfwGetFramebufferSize(window, &context.framebufferWidth,
                                       &context.framebufferHeight);

        Scene* scene = manager.currentScene();
        if (scene == nullptr) {
            break;  // nothing to run; createScene() already reported why
        }

        scene->handleInput(input);
        scene->update(deltaTime);

        renderer.beginFrame(context.framebufferWidth, context.framebufferHeight);
        scene->render();

        ++frameCount;

        // --screenshot mode: save the finished frame and stop.
        if (!options.screenshotPath.empty() &&
            frameCount >= options.framesBeforeScreenshot) {
            screenshot::saveFramebuffer(options.screenshotPath,
                                        context.framebufferWidth,
                                        context.framebufferHeight);
            glfwSwapBuffers(window);
            break;
        }

        glfwSwapBuffers(window);

        // Apply a scene change only now that the frame is fully presented.
        manager.applyPendingSwitch(context);
    }

    //-------------------------------------------------------------------------
    // Shutdown, in the reverse order things were created
    //-------------------------------------------------------------------------
    manager.shutdown();
    renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
