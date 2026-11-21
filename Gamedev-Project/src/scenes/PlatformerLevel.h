#pragma once
//=============================================================================
// PlatformerLevel - THE LAYOUT OF FLOOR 1. EDIT THIS FILE TO REDESIGN IT.
//
// Nothing here is code you have to understand: it is tables describing where
// things are. Add a row and the thing appears in the game.
//
// Coordinates: X is right, Y is up, Z is toward the camera. The level reads
// right to left - a ground run with pits and enemies, then a climb, then the
// goal. That shape is deliberate: a platformer wants a journey across, not
// just a ladder upward.
//
//-----------------------------------------------------------------------------
// THE SHAPE OF THE LEVEL: TWO FORKS
//
// The climb is not one path. Twice you arrive somewhere with two visible ways
// on, and have to pick one:
//
//     GROUND RUN  (shared - this is where every mechanic is taught)
//          |
//      FORK 1 ----------------------------+
//          |                              |
//     A: THE LONG STAIR              B: THE LAUNCH PAD
//     six wide steps, far side       three narrow ledges over the pit,
//     no enemies, no coins           a Spiker on the middle one, 3 coins
//          |                              |
//          +---------> MIDWAY LEDGE <-----+
//                          |
//      FORK 2 ----------------------------+
//          |                              |
//     C: THE WIDE PATH               D: THE NARROW LEAP
//     three wide platforms and a     four small ledges, long gaps,
//     slow moving platform,          a Walker to stomp, 3 coins
//     no enemies, no coins
//          |                              |
//          +----------> SUMMIT <----------+
//                          |
//                        GOAL
//
// The tradeoff is the same on both forks and is meant to be readable in one
// look: the SAFE branch is wide, slower and has nothing on it, the RISKY
// branch is short, narrow and holds every optional coin in the upper level.
// A player who only wants to finish never has to touch a risky branch; a
// player who wants all 16 coins has to take both. The signposts at each fork
// (kRouteSigns, below) say which is which before you commit.
//
// The branches are held far enough apart in Z that you cannot jump from a
// safe platform across onto a coin-carrying risky one - otherwise the safe
// route would quietly pay the risky route's reward and the decision would
// stop meaning anything. You CAN always drop off a risky branch onto the
// safe one, which is the bail-out: it costs you the coins you have not
// picked up yet, and that asymmetry is deliberate.
// tools/check_platformer_reach.py asserts both halves of this.
//
//-----------------------------------------------------------------------------
// DESIGN RULES, so new pieces stay reachable:
//   - A jump lifts you cfg::plat::kJumpHeight (2.3 units) straight up, so
//     never ask for a step taller than about 2.1.
//   - A running jump covers about 3.4 units horizontally while landing level
//     or slightly higher, and up to 4.0 landing at the same height. Those are
//     EDGE-TO-EDGE gaps, not centre-to-centre distances.
//   - A bounce pad throws you about 4.1 units up, and you travel roughly 4.0
//     units sideways over the whole arc.
//   - tools/check_platformer_reach.py re-checks every transition listed here.
//=============================================================================

#include <glm/vec3.hpp>

namespace level1 {

//-----------------------------------------------------------------------------
// STATIC PLATFORMS - the things you stand on.
//
// Grouped by which part of the level they belong to. The grouping is only for
// readability; the game treats them all alike.
//-----------------------------------------------------------------------------
struct PlatformDef {
    glm::vec3 center;
    glm::vec3 size;
    bool warm;   // true = warm accent color, false = cool. Decorative only.
};

inline constexpr PlatformDef kPlatforms[] = {
    //=========================================================================
    // SHARED: the ground run. The gaps between these are pits you must jump.
    //=========================================================================
    {{ -3.0f, -0.50f,  0.0f}, {12.0f, 1.00f, 7.0f}, false},  // G0  x -9.0 .. 3.0
    {{  9.0f, -0.50f,  0.0f}, { 6.0f, 1.00f, 7.0f}, false},  // G1  x  6.0 .. 12.0
    {{ 18.0f, -0.50f,  0.0f}, { 7.0f, 1.00f, 7.0f}, false},  // G2  x 14.5 .. 21.5

    //=========================================================================
    // FORK 1, BRANCH A - "THE LONG STAIR" (the safe one)
    //
    // Six wide steps away from the camera. Every gap here is nearly nothing
    // and every landing is 3.4 units across, so it is very hard to miss. It
    // costs you six jumps instead of three and carries no coins at all.
    //=========================================================================
    {{ 23.5f,  1.20f, -1.0f}, { 3.4f, 0.55f, 3.4f}, true },  // A1
    {{ 26.5f,  2.50f, -3.0f}, { 3.4f, 0.55f, 3.4f}, false},  // A2
    {{ 24.0f,  3.80f, -5.5f}, { 3.4f, 0.55f, 3.4f}, true },  // A3
    {{ 20.5f,  5.00f, -6.0f}, { 3.4f, 0.55f, 3.4f}, false},  // A4
    {{ 17.0f,  6.30f, -6.0f}, { 3.4f, 0.55f, 3.4f}, true },  // A5
    {{ 14.0f,  7.60f, -5.0f}, { 3.4f, 0.55f, 3.4f}, false},  // A6

    //=========================================================================
    // FORK 1, BRANCH B - "THE LAUNCH PAD" (the risky one)
    //
    // Three 2-unit ledges toward the camera, reached only off the bounce pad
    // on the ground. B2 hangs over the second pit, so a miss there is a real
    // fall, and it has a Spiker walking on it. Three jumps instead of six,
    // and it holds three coins.
    //=========================================================================
    {{ 15.6f,  3.60f,  2.4f}, { 2.2f, 0.50f, 2.2f}, true },  // B1
    {{ 12.4f,  5.20f,  2.4f}, { 2.0f, 0.50f, 2.0f}, false},  // B2  << Spiker
    {{  9.6f,  6.80f,  2.4f}, { 2.0f, 0.50f, 2.0f}, true },  // B3

    //=========================================================================
    // THE MIDWAY LEDGE - where fork 1 comes back together, and where fork 2
    // begins. Deliberately big: it is a place to stop and read the signs.
    //=========================================================================
    {{ 14.0f,  8.80f,  0.0f}, { 4.4f, 0.60f, 5.4f}, false},  // M

    //=========================================================================
    // FORK 2, BRANCH C - "THE WIDE PATH" (the safe one)
    //
    // Three wide platforms away from the camera, handing off to the slow
    // moving platform in kMovingPlatforms. Four steps, nothing on them.
    //=========================================================================
    {{ 11.0f, 10.00f, -2.2f}, { 3.4f, 0.55f, 3.4f}, true },  // C1
    {{  7.6f, 11.20f, -3.0f}, { 3.4f, 0.55f, 3.4f}, false},  // C2
    {{  4.2f, 12.40f, -2.2f}, { 3.2f, 0.55f, 3.2f}, true },  // C3

    //=========================================================================
    // FORK 2, BRANCH D - "THE NARROW LEAP" (the risky one)
    //
    // Four 2-unit ledges toward the camera with 1.5-1.8 unit gaps between
    // them, over a drop the whole way. The first jump up onto D1 is 1.95 of
    // the 2.3 a jump can lift you, so it has to be a committed one. A Walker
    // patrols D2 - stomp it and you bounce, walk into it and it costs a life.
    // Three coins.
    //=========================================================================
    {{  9.5f, 10.80f,  5.0f}, { 2.2f, 0.50f, 2.2f}, false},  // D1
    {{  5.8f, 11.90f,  5.0f}, { 2.0f, 0.50f, 2.0f}, true },  // D2  << Walker
    {{  2.0f, 13.00f,  5.0f}, { 2.0f, 0.50f, 2.0f}, false},  // D3
    {{ -1.6f, 13.90f,  2.6f}, { 2.2f, 0.50f, 2.2f}, true },  // D4  cuts back in

    //=========================================================================
    // THE SUMMIT - shared again. Both branches of fork 2 land on S1.
    //=========================================================================
    {{ -4.2f, 14.80f, -1.4f}, { 3.6f, 0.60f, 3.6f}, false},  // S1
    {{ -7.2f, 16.00f, -3.2f}, { 3.6f, 0.60f, 3.6f}, true },  // S2
    {{ -9.6f, 17.10f, -4.6f}, { 4.6f, 0.70f, 4.6f}, false},  // S3  goal sits here
};

inline constexpr int kPlatformCount =
    static_cast<int>(sizeof(kPlatforms) / sizeof(kPlatforms[0]));

//-----------------------------------------------------------------------------
// MOVING PLATFORMS - travel back and forth between two points.
//
// The one here is the last step of the SAFE branch of fork 2. It is slow and
// wide on purpose: waiting for it is the price of the safe route, and waiting
// is exactly the cost the risky route lets you skip.
//-----------------------------------------------------------------------------
struct MovingPlatformDef {
    glm::vec3 from;
    glm::vec3 to;
    glm::vec3 size;
    float speed;        // world units per second
    float startOffset;  // 0..1, where along the path it begins
};

inline constexpr MovingPlatformDef kMovingPlatforms[] = {
    // C4: carries you from the end of C3 across to the summit.
    {{0.8f, 13.40f, -2.0f}, {-3.2f, 13.40f, -2.0f}, {3.0f, 0.50f, 3.0f}, 2.2f, 0.0f},
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
//
// Note where these are: the ground run teaches the stomp twice and then tests
// the dodge, and the only two enemies above ground level are both on RISKY
// branches. Nothing on a safe branch can hurt you.
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
    // --- the ground run: introduce, practice, then test ---------------------
    {EnemyKind::Walker, -6.0f,  1.0f,  0.00f,  0.0f, 1.9f},  // introduce the stomp
    {EnemyKind::Walker,  6.8f, 11.2f,  0.00f,  0.0f, 2.3f},  // practice it, faster
    {EnemyKind::Spiker, 15.3f, 20.7f,  0.00f,  0.0f, 1.6f},  // test it: must be dodged

    // --- risky branches only ------------------------------------------------
    {EnemyKind::Spiker, 11.8f, 13.0f,  5.45f,  2.4f, 1.5f},  // on B2, 2 units wide
    {EnemyKind::Walker,  5.2f,  6.4f, 12.15f,  5.0f, 1.6f},  // on D2, stompable
};

inline constexpr int kEnemyCount =
    static_cast<int>(sizeof(kEnemies) / sizeof(kEnemies[0]));

//-----------------------------------------------------------------------------
// ITEM BLOCKS - solid, but punch the underside and a coin pops out.
//
// All four are on the shared ground run, which is the only place every player
// is guaranteed to go. The mechanic gets taught where nobody can miss it.
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
    // P0: off to the far side of the starting ground, with a coin overhead
    // that nothing else can reach. This is where the pad gets introduced, so
    // that at fork 1 the player already knows what one does.
    {{ 1.2f, 0.275f, -2.4f}, {2.2f, 0.55f, 2.2f}},

    // P1: the entrance to the RISKY branch of fork 1. Standing on the ground
    // next to it, the only thing above is B1 - so the pad reads as "that way".
    {{19.5f, 0.275f,  2.4f}, {2.6f, 0.55f, 2.6f}},
};

inline constexpr int kBouncePadCount =
    static_cast<int>(sizeof(kBouncePads) / sizeof(kBouncePads[0]));

//-----------------------------------------------------------------------------
// ROUTE SIGNS - the signposts that make the forks a decision rather than a
// guess. Purely informational: they are drawn in the world and labelled on
// screen, and you walk straight through them.
//
// A fork the player cannot read before committing is not a choice, it is a
// trap, so every branch entrance gets one of these.
//-----------------------------------------------------------------------------
enum class RouteRisk { Safe, Risky };

struct RouteSignDef {
    glm::vec3 base;      // where the post meets the floor
    RouteRisk risk;      // picks the panel color: calm green or hot amber
    const char* title;   // name of the branch
    const char* detail;  // what it costs and what it pays
};

inline constexpr RouteSignDef kRouteSigns[] = {
    // --- fork 1, on the far end of the ground run ---------------------------
    // Spread apart in X as well as Z: two signs on the same vertical line put
    // their screen labels on top of each other and neither can be read.
    {{21.0f, 0.00f, -2.8f}, RouteRisk::Safe,  "LONG STAIR",  "SAFE - 6 STEPS - NO COINS"},
    {{17.4f, 0.00f,  2.8f}, RouteRisk::Risky, "LAUNCH PAD",  "RISKY - 3 LEDGES - 3 COINS"},

    // --- fork 2, on the midway ledge ----------------------------------------
    {{12.8f, 9.10f, -2.3f}, RouteRisk::Safe,  "WIDE PATH",   "SAFE - SLOW LIFT - NO COINS"},
    {{15.4f, 9.10f,  2.3f}, RouteRisk::Risky, "NARROW LEAP", "RISKY - LONG GAPS - 3 COINS"},
};

inline constexpr int kRouteSignCount =
    static_cast<int>(sizeof(kRouteSigns) / sizeof(kRouteSigns[0]));

//-----------------------------------------------------------------------------
// COINS - walk into one to collect it.
//
// Where they are IS the reward structure, so it is worth stating plainly:
//   5 on the shared route  - everyone can get these
//   3 on risky branch B    - fork 1
//   3 on risky branch D    - fork 2
//   1 on the summit        - everyone
// Plus one inside each of the four item blocks. Sixteen in total, and ten of
// them are free; the other six are the reason to take a risk.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kCoins[] = {
    // --- shared: arcs over the two pits, so the jump pays ------------------
    {  3.90f,  1.80f,  0.0f},
    {  4.50f,  2.40f,  0.0f},
    {  5.10f,  1.80f,  0.0f},
    { 13.25f,  2.20f,  0.0f},

    // --- shared: hangs too high for a jump, to teach the bounce pad P0 -----
    {  1.20f,  3.60f, -2.4f},

    // --- risky branch B (fork 1), one over each ledge ----------------------
    { 15.60f,  4.80f,  2.4f},
    { 12.40f,  6.40f,  2.4f},
    {  9.60f,  8.00f,  2.4f},

    // --- risky branch D (fork 2), one over each of the first three ledges --
    {  9.50f, 12.00f,  5.0f},
    {  5.80f, 13.10f,  5.0f},
    {  2.00f, 14.20f,  5.0f},

    // --- shared: on the summit, just before the goal ------------------------
    { -7.20f, 17.25f, -3.2f},
};

inline constexpr int kCoinCount =
    static_cast<int>(sizeof(kCoins) / sizeof(kCoins[0]));

// Every coin in the level, including the ones hiding inside item blocks.
inline constexpr int kTotalCoins = kCoinCount + kItemBlockCount;

//-----------------------------------------------------------------------------
// START AND FINISH
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kSpawnPoint {-7.0f, 0.70f, 0.0f};
inline constexpr glm::vec3 kGoalPoint  {-9.6f, 17.80f, -4.6f};
inline constexpr glm::vec3 kGoalSize   {1.8f, 0.70f, 1.8f};

// Fall below this and you respawn at the last platform you stood on.
inline constexpr float kKillPlaneY = -14.0f;

} // namespace level1
