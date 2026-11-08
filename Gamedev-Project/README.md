# Roary's Adventure: Save Pawlina!

CAP 4052 group project.

## Team Members

Names: Pedro Re, Remberto Tejada

## Approved Concept


## Objective

Roary, FIU's panther, climbs a tower to rescue Pawlina. Each floor of the tower
is a level based on a different classic game genre. Clear a floor to unlock the
one above it, and reach the top to face the school mascots.

| Floor | Name | Goal |
| --- | --- | --- |
| 1 | Platform Peril | Run and jump across the platforms to reach the goal |
| 2 | The Shifting Tunnel | Reach the end of the tunnel without falling off |
| 3 | Silent Paws | Collect all four paws without getting caught, then exit through the door |
| Top | Rival Showdown | Beat Rocky the bull by making him crash into the wall, then hitting him while he's stunned |

---

## Controls

### Tower menu (level select)

| Key | Action |
| --- | --- |
| `W` / `S` or `Up` / `Down` | Move the selection up and down the tower |
| `Enter` / `Space` | Enter the selected floor |
| Mouse | Hover a floor to select it, click to enter |
| `Esc` | Quit |
| `U` | Debug: unlock every floor |

### Floor 1 - Platform Peril

| Key | Action |
| --- | --- |
| `W` `A` `S` `D` / arrows | Move |
| `Space` | Jump (hold for higher, tap for a hop) |
| `R` | Respawn |

### Floor 2 - The Shifting Tunnel

| Key | Action |
| --- | --- |
| `A` / `D` or `Left` / `Right` | Strafe |
| `Space` / `W` / `Up` | Jump |
| `R` | Restart the run |

### Floor 3 - Silent Paws

| Key | Action |
| --- | --- |
| `W` `A` `S` `D` / arrows | Move |
| `Shift` | Sneak (slower, but the guard can't see as far) |
| `E` | Throw a rock straight ahead |

### Top Floor - Rival Showdown

| Key | Action |
| --- | --- |
| `W` `A` `S` `D` / arrows | Move |
| `Space` | Dash (can't be hurt while dashing) |

### On every floor

| Key | Action |
| --- | --- |
| `Esc` | Back to the tower |
| `C` | Debug: mark the floor complete and unlock the next one |

---

## Implemented Features

### General
- Tower menu where each floor is a level; floors unlock in order
- Scene system that switches between the menu and the floors safely
- Mascot characters built from simple 3D shapes
- Built-in bitmap font for all on-screen text

### Floor 1 - Platform Peril
- 3D platforming with variable jump height
- Coyote time, jump buffering and jump cut for responsive controls
- Moving platforms that carry the player
- Enemies (walkers that can be stomped, spikers that can't), item blocks, bounce pads, coins
- Three lives with knockback and brief invulnerability after a hit

### Floor 2 - The Shifting Tunnel
- Auto-running down a square tunnel
- Running off the side of a surface moves the player onto the next wall, rotating gravity
- Jumping across gaps; falling restarts the run

### Floor 3 - Silent Paws
- Top-down movement with wall collision and wall sliding
- A guard that patrols a loop of waypoints, pausing at each one
- Vision cone: the guard sees Roary only when he's in range, inside the cone, and not behind a wall
- Being seen means getting caught: Roary goes back to the start, keeping collected paws
- Sneaking shortens how far the guard can see (the cone shrinks to match)
- Four collectible paws that unlock the exit door
- Throwable rocks: a rock flies straight ahead of Roary and lands at the first wall
  in the way (or at its maximum range)
- Noise distraction: a rock landing near the guard makes him look at it for a few
  seconds, then turn back and carry on along his route
- Feedback is kept minimal: the cone turns amber while the guard is distracted,
  the door turns green when it opens, and Roary flashes red when caught

### Top Floor - Rival Showdown
- Round arena with a dash that has invincibility and a cooldown
- Rocky the bull, driven by a state machine: Waiting, Telegraph, Charging, Stunned, Recover
- Telegraphed charges: Rocky faces Roary, then locks his aim and turns red while he
  winds up, then charges in a straight line
- Rocky is stunned (turns orange) when he hits the wall; touching him then damages him
- Hearts for Roary with brief invulnerability after a hit; the fight restarts after three hits
- Roary is tinted pale blue while he can't be hurt

---

## Technology Stack

| | |
| --- | --- |
| Language | C++20 |
| Graphics | OpenGL 3.3 (core profile) |
| Window and input | GLFW |
| OpenGL loader | GLAD |
| Math | GLM |
| Build system | CMake (3.24+) with presets |
| Package manager | vcpkg |

---

## Dependencies

All libraries are installed automatically by vcpkg from `vcpkg.json` the first
time the project is configured.

| Library | Used for |
| --- | --- |
| GLFW 3 | Creating the window, reading the keyboard and mouse |
| GLAD (OpenGL 3.3 loader) | Loading OpenGL functions |
| GLM | Vectors, matrices and other 3D math |

You also need:

- **CMake** 3.24 or newer
- **vcpkg**
- **macOS:** Ninja and a C++20 compiler (Xcode command line tools)
- **Windows:** Visual Studio 2022 with the "Desktop development with C++" workload (includes vcpkg)

---

## Build and Run Instructions

### macOS

1. Install CMake and Ninja, and clone vcpkg. Set `VCPKG_ROOT` to the vcpkg folder.
2. From the project folder:

```bash
cmake --preset mac
cmake --build --preset mac
./out/build/mac/roary
```

### Windows (Visual Studio 2022)

**Option A:** open the project folder in Visual Studio, choose the
`windows-x64` preset, then build and run `roary.exe`.

**Option B:** from a *Developer Command Prompt for VS 2022*, in the project folder:

```bat
cmake --preset windows-x64
cmake --build --preset windows-x64
out\build\windows-x64\Debug\roary.exe
```

The first configure downloads and builds the libraries and can take a few minutes.

### Optional command-line options (for testing)

```
--unlock-all     start with every floor unlocked
--floor <1-4>    start directly on that floor, skipping the menu
```

---

## Known Limitations

- This is a prototype: all characters and objects are simple shapes with flat
  colours, and there is no animation, sound or music
- Progress is not saved between sessions
- The built-in font is uppercase letters only
- Debug shortcuts (`C` to complete a floor, `U` to unlock all) are still enabled
- The camera is framed for a 1280x800 window; narrower windows crop the sides
- Floor 3:
  - only one guard
  - the vision cone is drawn through walls, though the guard can't actually see through them
  - the guard walks through Roary and never goes to investigate a noise
  - thrown rocks land instantly; there is no flight animation
- Top floor:
  - only Rocky is in the fight; Owlsley and Sebastian are not implemented yet
  - rescuing Pawlina at the end is not implemented yet

---

## External Asset Credits

No external art, models, textures, sounds or fonts are used. Every character and
object is built from basic shapes in code (with help from GenAI), and the on-screen font is a bitmap
font defined in `src/core/BitmapFont.h`.

Third-party libraries (see Dependencies) are used under their own open-source
licenses:

- GLFW: <https://www.glfw.org>
- GLAD: <https://github.com/Dav1dde/glad>
- GLM: <https://github.com/g-truc/glm>

Technique reference: the moving-platform ordering on Floor 1 follows Maddy
Thorson's Celeste / TowerFall physics write-up.

## Project Structure

```
src/
  main.cpp                   window, OpenGL setup and the main loop
  core/                      shared engine code (renderer, input, camera,
                             scene manager, progress, text, Config.h)
  scenes/
    LevelSelectScene         the tower menu
    PlatformerScene          Floor 1
    TunnelScene              Floor 2
    StealthScene             Floor 3
    BossScene                Top floor
```

All tuning values (sizes, speeds, colours, level layouts) are in
`src/core/Config.h`. Floor 1 and 2 layouts are in `PlatformerLevel.h` and
`TunnelLevel.h`.
