# Roary's Adventure: Save Pawlina!

CAP 4052 group project.

## Team Members

Names: Pedro Re, Remberto Tejada

Panther IDs:

## Approved Concept


## Objective

Roary, FIU's panther, climbs a tower to rescue Pawlina. Each floor of the tower
is a level based on a different classic game genre. Clear a floor to unlock the
one above it, and reach the top to face the school mascots.

Floor 1, Platform Peril: run and jump across the platforms to reach the goal.

Floor 2, The Shifting Tunnel: reach the end of the tunnel without falling off.

Floor 3, Silent Paws: collect all four paws without getting caught, then exit through the door.

Top Floor, Rival Showdown: beat Rocky the bull by making him crash into the wall, then touching him while he is stunned.

## Controls

### Tower Menu

W and S or Up and Down: move the selection up and down the tower

Enter or Space: enter the selected floor

Mouse: hover a floor to select it, click to enter

Esc: quit

U: debug, unlock every floor

### Floor 1: Platform Peril

W A S D or arrow keys: move

Space: jump, hold for a higher jump or tap for a hop

R: respawn

### Floor 2: The Shifting Tunnel

A and D or Left and Right: strafe

Space, W or Up: jump

R: restart the run

### Floor 3: Silent Paws

W A S D or arrow keys: move

Shift: sneak, slower but the guard cannot see as far

E: throw a rock straight ahead

### Top Floor: Rival Showdown

W A S D or arrow keys: move

Space: dash, Roary cannot be hurt while dashing

### On Every Floor

Esc: back to the tower

C: debug, mark the floor complete and unlock the next one

## Implemented Features

### General

Tower menu where each floor is a level, and floors unlock in order.

Scene system that switches between the menu and the floors safely.

Mascot characters built from simple 3D shapes.

Built in bitmap font for all on screen text.

### Floor 1: Platform Peril

3D platforming with variable jump height.

Coyote time, jump buffering and jump cut for responsive controls.

Moving platforms that carry the player.

Enemies, item blocks, bounce pads and coins. Walkers can be stomped, spikers cannot.

Three lives, with knockback and brief invulnerability after a hit.

### Floor 2: The Shifting Tunnel

Auto running down a square tunnel.

Running off the side of a surface moves the player onto the next wall, rotating gravity.

Jumping across gaps. Falling restarts the run.

### Floor 3: Silent Paws

Top down movement with wall collision and wall sliding.

A guard that patrols a loop of waypoints, pausing at each one.

Vision cone: the guard sees Roary only when he is in range, inside the cone, and not behind a wall.

Being seen means getting caught. Roary goes back to the start but keeps the paws he collected.

Sneaking shortens how far the guard can see, and the cone shrinks to match.

Four collectible paws that unlock the exit door.

Throwable rocks: a rock flies straight ahead of Roary and lands at its full range or at the first wall in the way.

Noise distraction: a rock landing near the guard makes him look at it for a few seconds, then turn back and carry on along his route.

Feedback is kept minimal. The cone turns amber while the guard is distracted, the door turns green when it opens, and Roary flashes red when caught.

### Top Floor: Rival Showdown

Round arena with a dash that has invincibility and a cooldown.

Rocky the bull, driven by a state machine with the states Waiting, Telegraph, Charging, Stunned and Recover.

Telegraphed charges: Rocky faces Roary, then locks his aim and turns red while he winds up, then charges in a straight line.

Rocky is stunned and turns orange when he hits the wall. Touching him then damages him.

Hearts for Roary, with brief invulnerability after a hit. The fight restarts after three hits.

Roary is tinted pale blue while he cannot be hurt.

## Technology Stack

Language: C++20

Graphics: OpenGL 3.3 core profile

Window and input: GLFW

OpenGL loader: GLAD

Math: GLM

Build system: CMake 3.24 or newer, with presets

Package manager: vcpkg

## Dependencies

All libraries are installed automatically by vcpkg from vcpkg.json the first
time the project is configured.

GLFW 3: creating the window and reading the keyboard and mouse

GLAD: loading the OpenGL 3.3 functions

GLM: vectors, matrices and other 3D math

You also need:

CMake: version 3.24 or newer

vcpkg: the package manager

macOS: Ninja and a C++20 compiler from the Xcode command line tools

Windows: Visual Studio 2022 with the Desktop development with C++ workload, which includes vcpkg

## Build and Run Instructions

### macOS

Install CMake and Ninja, clone vcpkg, and set VCPKG_ROOT to the vcpkg folder.
Then run these from the project folder:

```bash
cmake --preset mac
cmake --build --preset mac
./out/build/mac/roary
```

### Windows

Option A: open the project folder in Visual Studio 2022, choose the windows x64
preset, then build and run roary.exe.

Option B: open a Developer Command Prompt for VS 2022 in the project folder and run:

```bat
cmake --preset windows-x64
cmake --build --preset windows-x64
out\build\windows-x64\Debug\roary.exe
```

The first configure downloads and builds the libraries and can take a few minutes.

### Optional Testing Options

Add these after the program name when running it:

```
--unlock-all     start with every floor unlocked
--floor 3        start directly on that floor, skipping the menu
```

## Known Limitations

This is a prototype. All characters and objects are simple shapes with flat
colours, and there is no animation, sound or music.

Progress is not saved between sessions.

The built in font is uppercase letters only.

Debug shortcuts are still enabled: C completes a floor and U unlocks every floor.

The camera is framed for a 1280 by 800 window, so narrower windows crop the sides.

Floor 3: there is only one guard.

Floor 3: the vision cone is drawn through walls, though the guard cannot actually see through them.

Floor 3: the guard walks through Roary and never goes to investigate a noise.

Floor 3: thrown rocks land instantly, with no flight animation.

Top floor: only Rocky is in the fight. Owlsley and Sebastian are not implemented yet.

Top floor: rescuing Pawlina at the end is not implemented yet.

## External Asset Credits

No external art, models, textures, sounds or fonts are used. Every character and
object is built from basic shapes in code, with help from GenAI, and the on screen
font is a bitmap font defined in src/core/BitmapFont.h.

Third party libraries are used under their own open source licenses:

GLFW: https://www.glfw.org

GLAD: https://github.com/Dav1dde/glad

GLM: https://github.com/g-truc/glm

Technique reference: the moving platform ordering on Floor 1 follows Maddy
Thorson's Celeste and TowerFall physics write up.

## Project Structure

src/main.cpp: window, OpenGL setup and the main loop

src/core: shared engine code, including the renderer, input, camera, scene manager, progress, text and Config.h

src/scenes/LevelSelectScene: the tower menu

src/scenes/PlatformerScene: Floor 1

src/scenes/TunnelScene: Floor 2

src/scenes/StealthScene: Floor 3

src/scenes/BossScene: Top floor

All tuning values, such as sizes, speeds, colours and level layouts, are in
src/core/Config.h. The Floor 1 and Floor 2 layouts are in PlatformerLevel.h and
TunnelLevel.h.
