#pragma once
//=============================================================================
// PlatformerLevel - THE LAYOUT OF FLOOR 1. EDIT THIS FILE TO REDESIGN IT.
//
// Nothing here is code you have to understand: it is tables describing where
// things are. Add a row and the thing appears in the game.
//
// Coordinates: X is right, Y is up, Z is toward the camera. The level reads
// left to right - a ground run with pits and enemies, then a climb, then the
// goal. That shape is deliberate: a platformer wants a journey across, not
// just a ladder upward.
//
// DESIGN RULES, so new pieces stay reachable:
//   - A jump clears cfg::plat::kJumpHeight (2.3 units) straight up.
//   - A running jump covers about 3.4 units horizontally while landing level
//     or slightly higher, and up to 4.0 landing at the same height.
//   - A bounce pad throws you about 4.1 units up.
//=============================================================================

#include <glm/vec3.hpp>

namespace level1 {

//-----------------------------------------------------------------------------
// STATIC PLATFORMS - the things you stand on.
// Index order matters only for readability; the game treats them all alike.
//-----------------------------------------------------------------------------
struct PlatformDef {
    glm::vec3 center;
    glm::vec3 size;
    bool warm;   // true = warm accent color, false = cool. Decorative only.
};

inline constexpr PlatformDef kPlatforms[] = {
    // --- the ground run. The gaps between these are pits you must jump. ---
    {{ -3.0f, -0.50f,  0.0f}, {12.0f, 1.00f, 7.0f}, false},  // G0  x -9 .. 3
    {{  9.0f, -0.50f,  0.0f}, { 6.0f, 1.00f, 7.0f}, false},  // G1  x  6 .. 12
    {{ 18.0f, -0.50f,  0.0f}, { 7.0f, 1.00f, 7.0f}, false},  // G2  x 14.5 .. 21.5

    // --- the climb at the far end ---
    {{ 23.5f,  1.20f, -1.0f}, { 3.0f, 0.55f, 3.0f}, true },
    {{ 26.5f,  2.50f, -3.0f}, { 3.0f, 0.55f, 3.0f}, false},
    {{ 24.0f,  3.80f, -5.5f}, { 3.0f, 0.55f, 3.0f}, true },
    {{ 20.5f,  5.00f, -6.0f}, { 3.0f, 0.55f, 3.0f}, false},

    // --- the high route, reached only by the bounce pad ---
    {{ 14.0f,  9.20f, -4.0f}, { 3.2f, 0.55f, 3.2f}, true },
    {{ 11.0f, 10.40f, -2.0f}, { 3.0f, 0.55f, 3.0f}, false},
    {{  8.0f, 11.60f,  0.0f}, { 3.0f, 0.55f, 3.0f}, true },

    // --- the summit ---
    {{ -2.5f, 14.10f, -0.5f}, { 3.0f, 0.55f, 3.0f}, false},
    {{ -5.5f, 15.30f, -2.5f}, { 3.2f, 0.55f, 3.2f}, true },
    {{ -8.5f, 16.50f, -4.0f}, { 4.4f, 0.70f, 4.4f}, false},
};

inline constexpr int kPlatformCount =
    static_cast<int>(sizeof(kPlatforms) / sizeof(kPlatforms[0]));

//-----------------------------------------------------------------------------
// MOVING PLATFORMS - travel back and forth between two points.
//-----------------------------------------------------------------------------
struct MovingPlatformDef {
    glm::vec3 from;
    glm::vec3 to;
    glm::vec3 size;
    float speed;        // world units per second
    float startOffset;  // 0..1, where along the path it begins
};

inline constexpr MovingPlatformDef kMovingPlatforms[] = {
    {{4.5f, 12.80f, 0.5f}, {0.5f, 12.80f, 0.5f}, {2.8f, 0.50f, 2.8f}, 2.4f, 0.0f},
};

inline constexpr int kMovingPlatformCount =
    static_cast<int>(sizeof(kMovingPlatforms) / sizeof(kMovingPlatforms[0]));

//-----------------------------------------------------------------------------
// ENEMIES - patrol between minX and maxX at a fixed height.
//
//   Walker  waddles along; land on it to squash it and bounce off
//   Spiker  covered in spikes, so stomping it hurts. Go around it.
//
// Both hurt you if you walk into them from the side.
//-----------------------------------------------------------------------------
enum class EnemyKind { Walker, Spiker };

struct EnemyDef {
    EnemyKind kind;
    float minX;
    float maxX;
    float feetY;   // the surface it walks on
    float z;
    float speed;
};

inline constexpr EnemyDef kEnemies[] = {
    {EnemyKind::Walker, -6.0f,  1.0f, 0.0f,  0.0f, 1.9f},
    {EnemyKind::Walker,  6.8f, 11.2f, 0.0f,  0.0f, 2.3f},
    {EnemyKind::Spiker, 15.3f, 20.7f, 0.0f,  0.0f, 1.6f},  // must be dodged
    {EnemyKind::Walker, 19.4f, 21.6f, 5.275f, -6.0f, 1.7f},
    {EnemyKind::Walker,  6.9f,  9.1f, 11.875f, 0.0f, 1.8f},
};

inline constexpr int kEnemyCount =
    static_cast<int>(sizeof(kEnemies) / sizeof(kEnemies[0]));

//-----------------------------------------------------------------------------
// ITEM BLOCKS - solid, but punch the underside and a coin pops out.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kItemBlocks[] = {
    { -1.00f, 3.20f, 0.0f},
    {  0.40f, 3.20f, 0.0f},
    {  9.00f, 3.20f, 0.0f},
    { 18.00f, 3.20f, 0.0f},
};

inline constexpr int kItemBlockCount =
    static_cast<int>(sizeof(kItemBlocks) / sizeof(kItemBlocks[0]));

//-----------------------------------------------------------------------------
// BOUNCE PADS - land on one and it throws you far higher than a jump.
//-----------------------------------------------------------------------------
struct BouncePadDef {
    glm::vec3 center;
    glm::vec3 size;
};

inline constexpr BouncePadDef kBouncePads[] = {
    {{17.0f, 5.60f, -5.0f}, {2.4f, 0.55f, 2.4f}},   // the only way up to the high route
};

inline constexpr int kBouncePadCount =
    static_cast<int>(sizeof(kBouncePads) / sizeof(kBouncePads[0]));

//-----------------------------------------------------------------------------
// COINS - walk into one to collect it. Arcs over the pits reward the jump.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kCoins[] = {
    {  3.90f, 1.80f,  0.0f},   // over the first pit
    {  4.50f, 2.40f,  0.0f},
    {  5.10f, 1.80f,  0.0f},
    { 13.25f, 2.20f,  0.0f},   // over the second pit
    { 23.50f, 2.40f, -1.0f},
    { 24.00f, 5.00f, -5.5f},
    { 14.00f, 10.60f,-4.0f},
    {  8.00f, 12.80f, 0.0f},
    { -5.50f, 16.70f,-2.5f},
};

inline constexpr int kCoinCount =
    static_cast<int>(sizeof(kCoins) / sizeof(kCoins[0]));

// Every coin in the level, including the ones hiding inside item blocks.
inline constexpr int kTotalCoins = kCoinCount + kItemBlockCount;

//-----------------------------------------------------------------------------
// START AND FINISH
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kSpawnPoint {-7.0f, 0.70f, 0.0f};
inline constexpr glm::vec3 kGoalPoint  {-8.5f, 17.20f, -4.0f};
inline constexpr glm::vec3 kGoalSize   {1.8f, 0.70f, 1.8f};

// Fall below this and you respawn at the last platform you stood on.
inline constexpr float kKillPlaneY = -14.0f;

} // namespace level1
