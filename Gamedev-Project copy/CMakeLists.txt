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

    # --- core engine helpers -------------------------------------------------
    src/core/Shader.cpp
    src/core/Mesh.cpp
    src/core/Camera.cpp
    src/core/Input.cpp
    src/core/TextRenderer.cpp
    src/core/Renderer.cpp
    src/core/Screenshot.cpp
    src/core/GameProgress.cpp
    src/core/SceneManager.cpp

    # --- scenes --------------------------------------------------------------
    src/scenes/LevelSelectScene.cpp
    src/scenes/PlaceholderLevelScene.cpp
)

# Lets every file use includes like "core/Config.h" instead of long relative paths.
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

# Copy assets/ next to the executable so the game can use simple relative paths
# like "assets/roary.png" once real art replaces the placeholder primitives.
add_custom_command(TARGET roary POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:roary>/assets"
    COMMENT "Copying assets to output directory"
)
