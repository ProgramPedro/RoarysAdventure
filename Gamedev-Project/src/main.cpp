// Creates the window and OpenGL context, owns the long-lived objects (Renderer,
// GameProgress, SceneManager) and runs the frame loop: poll events, work out
// delta time, snapshot input, then hand handleInput, update and render to the
// current scene. The pending scene switch is applied last, after the frame has
// been presented, which is what lets a scene safely ask to switch at any point.
// Also parses the --screenshot, --frames, --floor and --unlock-all dev flags.

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
void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, std::max(width, 1), std::max(height, 1));
}

struct Options {
    std::string screenshotPath;
    int  framesBeforeScreenshot = 3;
    bool unlockAll = false;
    int  startFloor = 0;
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

}

int main(int argc, char** argv) {
    const Options options = parseCommandLine(argc, argv);

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW initialization failed.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        cfg::kWindowWidth, cfg::kWindowHeight, cfg::kWindowTitle, nullptr, nullptr);

    if (window == nullptr) {
        std::cerr << "Could not create a GLFW window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "GLAD could not load OpenGL functions.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';

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

    manager.requestScene(options.startFloor > 0
                             ? sceneIdForFloor(options.startFloor - 1)
                             : SceneId::LevelSelect);
    manager.applyPendingSwitch(context);

    double previousTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window) && !manager.quitRequested()) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        deltaTime = std::clamp(deltaTime, 0.0f, 0.1f);

        input.update(window);

        glfwGetFramebufferSize(window, &context.framebufferWidth,
                                       &context.framebufferHeight);

        Scene* scene = manager.currentScene();
        if (scene == nullptr) {
            break;
        }

        scene->handleInput(input);
        scene->update(deltaTime);

        renderer.beginFrame(context.framebufferWidth, context.framebufferHeight);
        scene->render();

        ++frameCount;

        if (!options.screenshotPath.empty() &&
            frameCount >= options.framesBeforeScreenshot) {
            screenshot::saveFramebuffer(options.screenshotPath,
                                        context.framebufferWidth,
                                        context.framebufferHeight);
            glfwSwapBuffers(window);
            break;
        }

        glfwSwapBuffers(window);

        manager.applyPendingSwitch(context);
    }

    manager.shutdown();
    renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
