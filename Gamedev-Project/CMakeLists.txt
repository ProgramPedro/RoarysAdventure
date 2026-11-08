# Builds the single `roary` executable from the core engine sources and the
# scenes, against OpenGL, GLFW, GLM and GLAD (restored by vcpkg). It adds src/ as
# an include root so files can use paths like "core/Config.h" instead of long
# relative ones, and copies assets/ next to the binary after linking.

cmake_minimum_required(VERSION 3.24)

project(RoarysAdventure VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(roary
    src/main.cpp

    src/core/Shader.cpp
    src/core/Mesh.cpp
    src/core/Camera.cpp
    src/core/Input.cpp
    src/core/TextRenderer.cpp
    src/core/Renderer.cpp
    src/core/MascotModels.cpp
    src/core/GameProgress.cpp
    src/core/SceneManager.cpp

    src/scenes/LevelSelectScene.cpp
    src/scenes/PlatformerScene.cpp
    src/scenes/TunnelScene.cpp
    src/scenes/BossScene.cpp
    src/scenes/StealthScene.cpp
)

target_include_directories(roary PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(roary PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    glad::glad
)

if(MSVC)
    target_compile_options(roary PRIVATE /W4 /permissive-)
else()
    target_compile_options(roary PRIVATE -Wall -Wextra -Wpedantic)
endif()
