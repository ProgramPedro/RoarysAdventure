# Roary's Adventure: Save Pawlina!

CAP 4052 group project. Roary, FIU's panther, climbs a tower to rescue Pawlina.
Each floor is a level based on a different classic game, and the top floor is a
boss fight against the rival school mascots.

The game currently opens on the **title / level-select screen**: the tower
itself is the menu. All four floors are now real, playable levels.

---

## Building and running

### macOS

Needs CMake, Ninja and a vcpkg checkout with `VCPKG_ROOT` set.

```bash
cmake --preset mac
cmake --build --preset mac
./out/build/mac/roary
```

### Windows (Visual Studio 2022)

Open the folder in Visual Studio and pick the `windows-x64` preset, or:

```bat
cmake --preset windows-x64
cmake --build --preset windows-x64
```

vcpkg restores GLFW, GLM and GLAD automatically from `vcpkg.json` the first
time you configure. That first configure can take a few minutes.

### Technology

C++20, OpenGL 3.3 core, GLFW (window + input), GLAD (function loader),
GLM (math), CMake + vcpkg. No other dependencies — the on-screen text uses a
font built into the source, so there is no font library and no `.ttf` to ship.

---

## Controls

### Level select (the tower)

| Key | Action |
| --- | --- |
| `W` / `Up`, `S` / `Down` | Move the selection up and down the tower |
| `Enter` / `Space` | Enter the selected floor |
| Mouse | Hover a floor to select it, click to enter |
| `Esc` | Quit |
| `U` | **Debug:** unlock every floor |

Only Floor 1 is unlocked at the start. Locked floors are dimmed, show a padlock
and cannot be entered — pressing Enter on one flashes the LOCKED message.

### Inside a placeholder level

| Key | Action |
| --- | --- |
| `Esc` | Back to the tower |
| `C` | **Debug:** mark this floor complete (unlocks the next one) and go back |

### Command-line options (development only)

The game needs no arguments to run normally. These exist to make testing and
grabbing screenshots for the report easier:

```
--screenshot <file>   render a few frames, save a PPM image, then exit
--frames <n>          frames to render before the screenshot (default 3)
--unlock-all          start with every floor unlocked
--floor <1-4>         open straight into that floor, skipping the menu
```

---

## Project layout

```
src/
  main.cpp                     window, GL context, the main loop
  core/
    Config.h                   >>> ALL colors, sizes, text and asset paths <<<
    Scene.h                    the interface every screen implements
    SceneId.h                  the list of screens, + floor index -> SceneId
    SceneContext.h             the shared services handed to each scene
    SceneManager.{h,cpp}       owns the current scene, switches safely
    GameProgress.{h,cpp}       which floors are unlocked / completed
    Renderer.{h,cpp}           shared shader + meshes + draw helpers
    Shader.{h,cpp}             compile/link, cached uniform setters
    Mesh.{h,cpp}               VAO/VBO wrapper; box / pyramid / cylinder /
                               sphere / fullscreen quad primitives
    Collision.h                AABB + per-axis movement, shared by the levels
    MascotModels.{h,cpp}       the character shapes, shared by every scene
    Camera.{h,cpp}             look-at camera, plus worldToScreen()
    Input.{h,cpp}              per-frame keyboard + mouse, with edge detection
    TextRenderer.{h,cpp}       screen-space text and UI rectangles
    BitmapFont.h               the 5x7 font data, one glyph per entry
    Screenshot.{h,cpp}         saves the framebuffer to a PPM (dev tool)
  scenes/
    LevelSelectScene.{h,cpp}   the tower menu and title screen
    PlatformerScene.{h,cpp}    FLOOR 1 - the 3D platformer
    PlatformerLevel.h          >>> FLOOR 1 LAYOUT - EDIT THIS TO REDESIGN IT <<<
    TunnelScene.{h,cpp}        FLOOR 2 - the gravity-shifting tunnel run
    TunnelLevel.h              >>> FLOOR 2 LAYOUT - EDIT THIS TO REDESIGN IT <<<
                               (floors 3 and 4 keep their layout in
                               Config.h instead, in the kF3 / kF4 sections)
    StealthScene.{h,cpp}       FLOOR 3 - sneak past the guard, open the door
    BossScene.{h,cpp}          TOP FLOOR - the dodging fight (in progress)
    Characters.{h,cpp}         Roary with walk animation, used by floors 3-4
    PlaceholderLevelScene.{h,cpp}  no longer routed to; kept as the template
                               to copy when a new screen is needed
assets/                        empty for now; real models and textures go here
```

### The tower

The tower is a round stack of drum-shaped floors. Each floor is a curved wall
built from flat segments (`kWallSegments`) with a gap left at the front
(`kWallOpeningDegrees`) so you can see into the room, capped above and below by
round floor discs. The **top floor has no ceiling**: it is an open castle roof
ringed by a low parapet with battlements, with the boss arena standing on a
raised dais in the middle.

Making the tower rounder or blockier is one number: raise or lower
`kWallSegments`. The whole silhouette is driven by `kTowerRadius`,
`kFloorHeight` and `kFloorGap`.

### The characters

Roary, Pawlina and the three rival mascots are built from rounded primitives -
squashed spheres and cylinders - because the real mascots are plush costumes
and that is what they look like. Their colors come from photographs of the
actual costumes, which matters more than it sounds: several are the opposite of
the obvious guess.

| Mascot | Body | Kit | Signature feature |
| --- | --- | --- | --- |
| Roary / Pawlina | tan panther fur | FIU navy | Pawlina's gold-trimmed dress and bow |
| FAU Owlsley | **dark brown** | FAU blue | bright **orange brows** over blue eyes |
| Miami Sebastian | **white** | Miami orange | long orange bill, yellow feet, sailor hat |
| USF Rocky | **grey** | USF green | large curved **gold horns** |

Every one of those colors is a named constant in the mascot section of
`Config.h`.

### Floor 1 - Platform Peril

A 3D platformer. Run and jump up a tower of platforms to the goal, collecting
coins. WASD to move, Space to jump (hold for height, tap for a hop), R to
respawn, Esc to leave, C to force-complete.

**Changing the level** is `src/scenes/PlatformerLevel.h` and nothing else. It
is three tables - static platforms, moving platforms, coins - plus the spawn
and goal. Add a row and the thing appears.

**Changing the feel** is `cfg::plat` in `Config.h`. Jump is described the way a
designer thinks about it: `kJumpHeight` (how high) and `kTimeToApex` (how long
to get there). Gravity and launch speed are *derived* from those, so a jump
always reaches exactly the stated height - you never tune a gravity constant by
trial and error.

**Mechanics**

| Thing | What it does |
| --- | --- |
| Walker | patrols a set range; land on it to squash it and bounce off |
| Spiker | spiked on top, so stomping hurts - it has to be dodged |
| Item block | solid, but punch the underside and a coin pops out |
| Bounce pad | throws you ~4.1 units up, far past a normal 2.3 jump |
| Lives | three; a side hit costs one, knocks you back and grants brief invulnerability |

The level reads left to right - a ground run with pits and enemies, then a
climb, then the summit. That shape is deliberate: the genre wants a journey
across, not a ladder upward.

Three details that matter more than they look:

- **Coyote time** - you can still jump for ~0.12s after walking off an edge.
- **Jump buffering** - a press up to ~0.12s before landing still fires.
- **Jump cut** - releasing early while rising shortens the jump.

Without these the controls feel broken even though they are technically
correct. They are the difference between a platformer that reads as finished
and one that reads as a prototype.

**Moving platforms** follow the ordering from Maddy Thorson's Celeste /
TowerFall write-up: work out who is riding *before* the platform moves, move
all platforms before any actors, and carry riders by the exact platform
movement with no collision check. Done in any other order you get jitter,
sinking, or being flung off.

### Floor 2 - The Shifting Tunnel

You auto-run down a square tunnel. Run off the side of the surface you are on
and you carry on across the neighbouring wall, which becomes your new floor -
the world holds still and your sense of down rotates. A/D to strafe, Space to
jump, R to restart. Miss a jump and the whole run starts over - the tunnel is
short enough that a restart is a setback rather than a punishment.

**Changing the level** is `src/scenes/TunnelLevel.h`. Each row is one segment
of tunnel and lists which tiles exist on each of the four faces, three lanes
each, as `#` and `.`:

```
{"..#", "###", "...", "..."}     // floor only on the right; wall available
{"...", "###", "...", "..."}     // floor gone - you must be on the wall
```

Face 0 is the floor and face + 1 is always whichever face you reach by
strafing RIGHT on screen, so the corners pair up as face 0 lane 2 to face 1
lane 0, face 1 lane 2 to face 2 lane 0, and so on round the tube; so to push the player onto a wall you leave
the adjacent lane solid and then delete the floor.

**How the orientation is stored matters.** The player is on one of four faces,
held as an integer 0-3, and gravity points at whichever that is. Only the
camera's roll angle interpolates, purely for looks. The usual way to build
this - applying forces and letting a free rotation integrate - is documented
as drifting over time. Four discrete states cannot drift, by construction.

**Is it beatable?** There is a solvability checker: it does a breadth-first
search over every (segment, face, lane) the player could stand on, allowing a
one-lane shift per segment, corner transitions, and a jump clearing about two
segments. If it cannot reach the last segment, the level is impossible. Run it
after editing the layout.

### Where to change how things look

**`src/core/Config.h` is the only file you need to touch for art direction.**
Every color, every size, the camera placement, the floor names and descriptions,
and the asset paths are all there as named constants. Nothing else in the
project hardcodes a color or a measurement.

The background and the UI are FIU blue and gold. The tower deliberately uses its
own palette — weathered stone with a per-floor accent — so it reads as a
separate object instead of blending into the backdrop. Roary himself is the one
exception: he keeps FIU colors, because he is the mascot.

---

## How the scene system works

### The frame

`main.cpp` does not know about any particular screen. Each frame it runs:

1. `glfwPollEvents()`
2. work out delta time (clamped, so a dragged window does not produce one huge frame)
3. `input.update(window)` — snapshots the keyboard and mouse
4. `scene->handleInput(input)`
5. `scene->update(deltaTime)`
6. `scene->render()`
7. `glfwSwapBuffers(window)`
8. `manager.applyPendingSwitch(context)`

### Switching scenes without breaking the frame

A scene must **never** delete itself. If it did, the rest of that frame would be
running on a destroyed object.

Instead a scene calls:

```cpp
ctx().manager.requestScene(SceneId::LevelSelect);
```

That only writes down which scene we want next — nothing is created or destroyed
yet, so the current frame finishes normally. Step 8 above, which runs *after*
the frame has been drawn and presented, is where `SceneManager` actually runs the
old scene's `cleanup()`, destroys it, builds the new scene, attaches the context
and calls `init()`.

This means it is safe to request a switch from inside `handleInput()`,
`update()` or even `render()`, and the requesting scene can carry on for the rest
of the frame as if nothing happened. (There is a headless test of exactly this
behaviour — see "Verification" below.)

### Shared state: `SceneContext`

Every scene gets a `SceneContext&` holding things that outlive all scenes:

```cpp
ctx().window              // the GLFW window
ctx().framebufferWidth    // live size, refreshed every frame
ctx().framebufferHeight
ctx().renderer            // shared shader, meshes, text drawing
ctx().progress            // GameProgress: unlocked / completed state
ctx().manager             // call requestScene() / requestQuit() on this
```

Because these are references to objects owned by `main()`, they are valid from
`init()` all the way through `cleanup()`. There are no globals.

---

## Adding a real level

Say you are replacing the Floor 1 placeholder with a real platformer.

**1. Write the scene.** Subclass `Scene` and implement the five lifecycle
methods. `PlaceholderLevelScene` is the smallest possible working example to
copy from.

```cpp
// src/scenes/PlatformerScene.h
#include "core/Scene.h"

class PlatformerScene : public Scene {
public:
    bool init() override;
    void handleInput(const Input& input) override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;          // only if init() allocated something
    const char* name() const override { return "PlatformerScene"; }
};
```

**2. Register it.** `SceneManager::createScene()` in `SceneManager.cpp` is the
only place that knows about concrete scene classes. Change one line:

```cpp
case SceneId::Floor1: return std::make_unique<PlatformerScene>();
```

If the new screen is not one of the four floors (a pause menu, a credits
screen), add an entry to `SceneId` in `SceneId.h` as well.

**3. Report completion.** When the player actually beats the level, make the
same call the placeholder's debug `C` key makes:

```cpp
ctx().progress.markCompleted(0);                       // 0-based floor index
ctx().manager.requestScene(SceneId::LevelSelect);      // back to the tower
```

`markCompleted()` marks the floor cleared **and unlocks the floor above it**.
The level-select screen reads `GameProgress` fresh every frame, so the newly
unlocked floor and the updated "FLOORS CLEARED" counter appear immediately with
no extra wiring. Calling it on the top floor is safe — the bounds check handles
having nothing above.

**4. Add the `.cpp` to `CMakeLists.txt`.**

That is the whole integration. Nothing else needs editing.

### Things your scene gets for free

```cpp
Renderer& r = ctx().renderer;

r.drawBackground(topColor, bottomColor, glowColor, glowPos, radius, strength);
r.setCamera(camera.view(), camera.perspective(width, height));
r.drawBoxAt(center, size, color);                 // also drawBox(modelMatrix, ...)
r.drawPyramidAt(center, size, color);
r.setDepthWrite(false);                           // around translucent draws

TextRenderer& t = r.text();
t.begin(width, height);                           // pixel space, origin top-left
t.drawCentered("HELLO", centerX, y, scale, color);
t.fillRect(x, y, w, h, color, alpha);
t.end();
```

Input uses edge detection, which matters for menus:

```cpp
input.isDown(GLFW_KEY_A)        // held - use for movement
input.wasPressed(GLFW_KEY_A)    // only the frame it went down - use for menus
input.mousePosition()           // framebuffer pixels, Retina-corrected
input.wasMouseClicked()
```

### A note on UI sizing

Text sizes and UI positions are written in "design pixels" against an 800-pixel
tall window, then multiplied by `framebufferHeight / cfg::kUiReferenceHeight`.
Without that, every label would come out half-size on a Retina display, where
the framebuffer is twice the window size. Both existing scenes do this with a
small `px()` helper — copy the pattern in any new scene that draws text.

---

## Verification

The build is clean with `-Wall -Wextra -Wpedantic` (MSVC: `/W4 /permissive-`).

The scene system was checked headlessly against the real `SceneManager`,
`GameProgress` and scene classes — 20 assertions covering the starting lock
state, the deferred switch (a scene survives the rest of its frame after
requesting a switch), the unlock chain, out-of-range floor indices, `reset()`,
`unlockAll()`, and that every floor's `SceneId` builds a scene. All passed.

## Swapping in a real font

The 5x7 font in `BitmapFont.h` is uppercase-only, which suits a title screen but
would be limiting for paragraphs. To move to real type, replace the internals of
`TextRenderer` with stb_truetype: bake an atlas in `init()`, emit one textured
quad per character in `draw()`. The public interface stays the same, so no scene
code has to change. You would add `"stb"` to `vcpkg.json` and commit a
redistributable `.ttf` (DejaVu or Liberation) to `assets/`.
