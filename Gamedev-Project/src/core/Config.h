#pragma once
//=============================================================================
// Config.h - THE ONE FILE TO EDIT FOR LOOK AND FEEL
//
// Every color, size, position, speed and asset path used by the game lives in
// this file. Nothing else in the project hardcodes a color or a measurement.
// If you want to retheme the game or swap placeholder shapes for real art,
// start here.
//
// Colors are linear RGB in the 0..1 range. The hex value in the comment is the
// matching web color so it is easy to compare against a style guide.
//=============================================================================

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace cfg {

//-----------------------------------------------------------------------------
// Window
//-----------------------------------------------------------------------------
const int  kWindowWidth   = 1280;
const int  kWindowHeight  = 800;
const char kWindowTitle[] = "Roary's Adventure: Save Pawlina!";

//-----------------------------------------------------------------------------
// FIU-THEMED BACKGROUND  (blue and gold - the school colors)
//
// The background and the on-screen text are the FIU-branded part of the game.
// The tower itself deliberately uses its OWN palette further down so it reads
// as a separate object instead of blending into the backdrop.
//-----------------------------------------------------------------------------
const glm::vec3 kBgTop     {0.031f, 0.118f, 0.247f}; // #081E3F FIU Blue
const glm::vec3 kBgBottom  {0.008f, 0.035f, 0.086f}; // #020916 deeper navy
const glm::vec3 kBgGlow    {0.714f, 0.525f, 0.173f}; // #B6862C FIU Gold
const glm::vec2 kBgGlowPos {0.0f, 0.35f};            // in NDC (-1..1)
const float     kBgGlowRadius   = 1.15f;
const float     kBgGlowStrength = 0.55f;

// FIU gold / blue reused by the UI text and by Roary himself.
const glm::vec3 kFiuGold {0.839f, 0.682f, 0.216f};  // #D6AE37 brighter gold
const glm::vec3 kFiuBlue {0.090f, 0.251f, 0.494f};  // #17407E lighter blue

//-----------------------------------------------------------------------------
// UI TEXT
//-----------------------------------------------------------------------------
const glm::vec3 kTextPrimary   {0.960f, 0.960f, 0.980f}; // near-white
const glm::vec3 kTextDim       {0.600f, 0.640f, 0.720f}; // hints
const glm::vec3 kTextLocked    {0.820f, 0.380f, 0.380f}; // LOCKED label
const glm::vec3 kTextComplete  {0.380f, 0.880f, 0.560f}; // COMPLETE label

// Text sizes are "how many screen pixels per font pixel" (the font is 5x7).
const float kTitleScale       = 7.0f;
const float kSubtitleScale    = 4.0f;
const float kFloorNameScale   = 4.0f;
const float kDescriptionScale = 2.0f;
const float kHintScale        = 2.0f;
const float kFloorLabelScale  = 2.0f;

//-----------------------------------------------------------------------------
// TOWER PALETTE  (intentionally NOT FIU blue/gold)
//
// The tower is weathered stone with a per-floor accent color, so each floor
// hints at the game it is based on.
//-----------------------------------------------------------------------------
const glm::vec3 kTowerStone     {0.290f, 0.306f, 0.341f}; // #4A4E57 slate
const glm::vec3 kTowerStoneDark {0.192f, 0.204f, 0.235f}; // #31343C shadow
const glm::vec3 kTowerBase      {0.235f, 0.212f, 0.192f}; // #3C3631 foundation
const glm::vec3 kTowerMerlon    {0.482f, 0.506f, 0.553f}; // #7B818D battlements

// Locked floors are faded toward this color by kLockedFade (0 = normal, 1 = fully faded).
const glm::vec3 kLockedGrey = {0.180f, 0.188f, 0.204f};
const float     kLockedFade = 0.60f;

// The pulsing frame drawn around the currently selected floor.
const glm::vec3 kSelectFrame      {1.000f, 0.960f, 0.870f}; // warm white
const float     kSelectPulseSpeed = 3.2f;
const float     kSelectFrameThick = 0.12f;

//-----------------------------------------------------------------------------
// TOWER GEOMETRY
//
// Floors are stacked along +Y. Floor 0 is the bottom floor, floor 3 is the top.
//-----------------------------------------------------------------------------
const int   kFloorCount   = 4;
const float kFloorHeight  = 2.30f;  // Y size of one floor
const float kFloorGap     = 0.12f;  // visible seam between floors
const float kTowerBaseY   = 0.0f;   // Y of the bottom of floor 0

// The only two constexpr values left in this file
//   - LevelSelectScene.cpp builds its own constexpr kRadius from kTowerRadius
//   - a constexpr can only be built from other constexpr values, and a plain
//     const float doesn't count, so these two have to stay constexpr
//   - kTowerRadius is built from kFloorWidth, so that one stays too
//   - the tower is ROUND, so its size is a radius instead of a width and depth
inline constexpr float kFloorWidth  = 7.0f;   // full width of the tower
inline constexpr float kTowerRadius = kFloorWidth * 0.5f;

// Y coordinate of the CENTER of a given floor.
inline float floorCenterY(int floorIndex) {
    return kTowerBaseY + kFloorHeight * 0.5f
         + static_cast<float>(floorIndex) * (kFloorHeight + kFloorGap);
}

//-----------------------------------------------------------------------------
// TOWER CONSTRUCTION DETAIL  (the cutaway look of the level-select tower)
//
// Each floor is drawn as an open-fronted room: a back wall in the floor's theme
// color, stone slabs above and below it, and stone pillars at the sides. The
// props that hint at each floor's game sit in front of the back wall.
//-----------------------------------------------------------------------------
const float kSlabThickness = 0.22f;  // floor / ceiling discs
const float kPillarWidth   = 0.42f;  // columns framing the opening
const float kWallThickness = 0.45f;  // thickness of the curved wall
const float kPropZ         = 0.0f;   // props sit on the tower axis

// The curved wall is drawn as a ring of flat segments. More segments looks
// rounder; 18 across the covered arc is plenty at this size.
const int   kWallSegments  = 18;

// How much of the circle is left open at the front so you can see inside,
// centered on +Z (toward the camera).
const float kWallOpeningDegrees = 112.0f;

// Discs stick out past the wall a little, giving each floor a lip.
const float kSlabOverhang = 0.28f;

//-----------------------------------------------------------------------------
// THE OPEN CASTLE TOP
//
// The highest floor has no ceiling. Instead it is ringed by a low parapet with
// battlements, so the boss fight happens on an open roof under the sky.
//-----------------------------------------------------------------------------
// Sized so the merlons and the gaps between them are about equal, which is
// what makes a battlement read as a battlement instead of a bumpy rim.
const float kParapetHeight = 0.55f;  // the solid low wall
const float kMerlonHeight  = 0.62f;  // the blocks on top of it
const float kMerlonWidth   = 0.78f;
const int   kMerlonCount   = 10;     // spaced around the covered arc

// The boss arena stands on a low round dais. Besides looking like a proper
// arena, it lifts the figures clear of the deck's rim, which would otherwise
// hide their legs from a camera looking slightly up at the tower.
const float kBossDaisHeight = 0.40f;
const float kBossDaisRadius = 3.00f;

// How tall the top floor COUNTS as for the selection ring and mouse picking.
// Its room is open, so using the full floor height would make you able to
// select it by pointing at empty sky.
const float kTopFloorPickHeight = 1.90f;

//-----------------------------------------------------------------------------
// UI SCALING
//
// All UI positions and text sizes in the scenes are written in "design pixels"
// against this reference window height, then multiplied by
// (framebufferHeight / kUiReferenceHeight). That keeps the interface the same
// apparent size on a normal display and on a Retina display, where the
// framebuffer is twice the window size.
//-----------------------------------------------------------------------------
const float kUiReferenceHeight = 800.0f;

//-----------------------------------------------------------------------------
// CAMERA  (level-select screen)
//
// A gentle three-quarter side view: mostly side-on so the tower reads as a
// stack of floors, but angled just enough to show depth.
//-----------------------------------------------------------------------------
// Framed so the tower spans about 55% of the window height, sitting just
// below the title and just above the info panel. The target sits a little
// BELOW the tower's center, which pushes the tower slightly up the screen.
const glm::vec3 kMenuCameraPos    {5.8f, 4.9f, 25.8f};
const glm::vec3 kMenuCameraTarget {-0.2f, 2.62f, 0.0f};
const float     kMenuCameraFovDeg = 42.0f;
const float     kCameraNear       = 0.1f;
const float     kCameraFar        = 100.0f;

// Very subtle idle drift so the menu is not perfectly static. Set to 0 to stop.
const float kCameraSwayAmount = 0.30f;
const float kCameraSwaySpeed  = 0.35f;

// Direction the single scene light travels (it is normalized at use).
const glm::vec3 kLightDirection {-0.45f, -0.80f, -0.40f};

//-----------------------------------------------------------------------------
// MASCOT COLORS
//
// Taken from photographs of the real costumes, so the characters are
// recognisable rather than generically school-colored. A few notes, because
// several of these are the opposite of what you would guess:
//   - the FAU Owl is dark BROWN with bright orange brows; blue is only his kit
//   - Miami's Ibis is WHITE with an orange bill; orange is only his kit
//   - USF's Bull is GREY with gold horns; green is only his kit
//   - Roary and Pawlina are tan panthers; navy is their uniform
//-----------------------------------------------------------------------------

// Shared panther fur, worn by both Roary and Pawlina.
const glm::vec3 kPantherFur   {0.784f, 0.576f, 0.333f}; // #C89355
const glm::vec3 kPantherCream {0.910f, 0.824f, 0.682f}; // #E8D2AE muzzle
const glm::vec3 kPantherInner {0.549f, 0.369f, 0.200f}; // #8C5E33 inner ear
const glm::vec3 kPantherNose  {0.227f, 0.165f, 0.133f}; // #3A2A22
const glm::vec3 kEyeDark      {0.102f, 0.094f, 0.110f}; // #1A181C pupils
const glm::vec3 kEyeWhite     {0.965f, 0.961f, 0.953f};

// The FIU kit both panthers wear.
const glm::vec3 kFiuKit       {0.086f, 0.145f, 0.353f}; // #16255A navy
const glm::vec3 kFiuKitTrim   {0.950f, 0.950f, 0.960f}; // white lettering
const glm::vec3 kPawlinaTrim  {0.788f, 0.635f, 0.153f}; // #C9A227 gold hem

// FAU's Owlsley.
const glm::vec3 kOwlBody   {0.290f, 0.204f, 0.149f}; // #4A3426 dark brown
const glm::vec3 kOwlCream  {0.847f, 0.773f, 0.659f}; // #D8C5A8 beak, speckles
const glm::vec3 kOwlBrow   {0.949f, 0.361f, 0.098f}; // #F25C19 signature orange
const glm::vec3 kOwlEye    {0.180f, 0.545f, 0.831f}; // #2E8BD4 blue
const glm::vec3 kOwlJersey {0.169f, 0.333f, 0.722f}; // #2B55B8 FAU blue, lifted
const glm::vec3 kOwlPants  {0.925f, 0.925f, 0.937f}; // white

// Miami's Sebastian.
const glm::vec3 kIbisBody   {0.949f, 0.941f, 0.918f}; // #F2F0EA white
const glm::vec3 kIbisBill   {0.910f, 0.384f, 0.165f}; // #E8622A orange
const glm::vec3 kIbisJersey {0.957f, 0.451f, 0.129f}; // #F47321 Miami orange
const glm::vec3 kIbisFeet   {0.949f, 0.761f, 0.188f}; // #F2C230 yellow
const glm::vec3 kIbisGreen  {0.000f, 0.314f, 0.188f}; // #005030 hat band

// USF's Rocky.
const glm::vec3 kBullBody   {0.608f, 0.608f, 0.588f}; // #9B9B96 grey
const glm::vec3 kBullMuzzle {0.749f, 0.741f, 0.714f}; // #BFBDB6
const glm::vec3 kBullHorn   {0.831f, 0.686f, 0.216f}; // #D4AF37 gold
const glm::vec3 kBullJersey {0.043f, 0.478f, 0.310f}; // #0B7A4F USF green, lifted
const glm::vec3 kBullPants  {0.812f, 0.769f, 0.576f}; // #CFC493 vegas gold
const glm::vec3 kBullHoof   {0.129f, 0.125f, 0.133f};

//-----------------------------------------------------------------------------
// ROARY MARKER  (the panther who rides the lift up and down the tower)
//-----------------------------------------------------------------------------
const glm::vec3 kRoaryAccent = kFiuGold;  // his lift platform
const glm::vec3 kRoarySize   {1.00f, 1.42f, 1.00f};

// Roary stands to the LEFT of the tower and slides up/down to the selection.
const float kRoaryOffsetX   = -kTowerRadius - 1.25f;
const float kRoaryOffsetZ   = 1.1f;
const float kRoaryMoveSpeed = 8.0f;   // higher = snappier slide
const float kRoaryBobAmount = 0.09f;
const float kRoaryBobSpeed  = 2.6f;

//-----------------------------------------------------------------------------
// PER-FLOOR THEME COLORS
//-----------------------------------------------------------------------------
// Floor 1 - Mario-style platformer: bright, cheerful blocks.
const glm::vec3 kF1Wall      {0.333f, 0.447f, 0.600f}; // #556F99 bright blue-grey
const glm::vec3 kF1BlockWarm {1.000f, 0.478f, 0.302f}; // #FF7A4D coral
const glm::vec3 kF1BlockCool {0.310f, 0.878f, 0.627f}; // #4FE0A0 mint
const glm::vec3 kF1Coin      {1.000f, 0.835f, 0.310f}; // #FFD54F coin yellow

// Floor 2 - "Run"-style shifting tunnel: near-black with a violet rim.
const glm::vec3 kF2Wall    {0.078f, 0.047f, 0.118f}; // #140C1F near black
const glm::vec3 kF2Tunnel  {0.027f, 0.016f, 0.047f}; // #07040C void
const glm::vec3 kF2RimGlow {0.420f, 0.247f, 0.627f}; // #6B3FA0 violet
const glm::vec3 kF2Accent  {0.541f, 0.886f, 0.937f}; // #8AE2EF cyan edge

// Floor 3 - Stealth: dark teal room, pale searchlights, black guards.
const glm::vec3 kF3Wall        {0.082f, 0.200f, 0.227f}; // #15333A dark teal
const glm::vec3 kF3Light       {0.812f, 0.965f, 1.000f}; // #CFF6FF searchlight
const glm::vec3 kF3Guard       {0.047f, 0.059f, 0.071f}; // #0C0F12 silhouette
const float     kF3LightAlpha  = 0.34f;
const float     kF3SweepSpeed  = 0.9f;
const float     kF3SweepDeg    = 26.0f;  // half-angle of the sweep

// Floor 3 - stealth level (StealthScene)
// The room is centred on the origin; walls sit at +/- kF3RoomHalfX and kF3RoomHalfZ
const float kF3RoomHalfX  = 16.0f;
const float kF3RoomHalfZ  = 10.5f;
const float kF3WallHeight = 1.80f;
const float kF3WallThick  = 0.50f;

const float kF3NearWallHeight = 0.34f;

const float kF3InnerWallHeight = 1.15f;

struct F3WallSlab {
    float centerX;
    float centerZ;
    float sizeX;
    float sizeZ;
};

// Interior walls: centre and size of each rectangle. Used for drawing AND collision.
const F3WallSlab kF3InnerWalls[] = {
    { -4.0f, -4.75f, kF3WallThick, 11.5f },
    { -4.0f,  7.25f, kF3WallThick,  6.5f },

    {  6.0f, -8.50f, kF3WallThick,  4.0f },
    {  6.0f,  3.50f, kF3WallThick, 14.0f },

    {  8.50f, 0.0f,  5.0f, kF3WallThick },
    { 15.00f, 0.0f,  2.0f, kF3WallThick },
};

const int kF3InnerWallCount =
    static_cast<int>(sizeof(kF3InnerWalls) / sizeof(kF3InnerWalls[0]));

const glm::vec3 kF3Floor  {0.063f, 0.129f, 0.149f};
const glm::vec3 kF3WallIn {0.110f, 0.235f, 0.267f};

const float kF3PlayerRadius = 0.42f;
const float kF3PlayerScale  = 1.55f;
const float kF3WalkSpeed    = 5.20f;
const float kF3SneakSpeed   = 2.30f;

const glm::vec3 kF3StartPos {-kF3RoomHalfX + 2.0f, 0.0f, kF3RoomHalfZ - 2.0f};

const glm::vec3 kF3GuardBody {0.188f, 0.208f, 0.247f};
const glm::vec3 kF3GuardHead {0.251f, 0.275f, 0.318f};

const float kF3GuardRadius     = 0.45f;
const float kF3GuardBodyHeight = 1.45f;
const float kF3GuardHeadSize   = 0.52f;

const float kF3VisionRange        = 9.0f;
const float kF3VisionHalfAngleDeg = 30.0f;
const float kF3SneakVisionMul     = 0.6f;   // vision range while Roary sneaks

const float kF3ConeApexY  = 0.26f;
const float kF3ConeHeight = 0.40f;

const glm::vec3 kF3ConeCalm  {0.675f, 0.886f, 0.937f};
const float     kF3ConeAlpha = 0.20f;

const glm::vec3 kF3ItemPositions[] = {
    { -12.5f, 0.0f, -7.0f },
    {  -1.5f, 0.0f,  7.0f },
    {  11.5f, 0.0f,  6.0f },
    {  12.0f, 0.0f, -6.5f },
};

const int kF3ItemCount =
    static_cast<int>(sizeof(kF3ItemPositions) / sizeof(kF3ItemPositions[0]));

const float kF3ItemPickupRadius = 1.10f;

const glm::vec3 kF3ItemPaw    = kFiuGold;
const float     kF3ItemHoverY = 0.80f;
const float     kF3ItemSize   = 0.80f;

const float kF3DoorZ      = 6.00f;
const float kF3DoorWidth  = 2.60f;
const float kF3DoorHeight = 1.45f;

const float kF3DoorTriggerRadius = 1.80f;

const glm::vec3 kF3DoorLocked {0.451f, 0.149f, 0.173f};
const glm::vec3 kF3DoorOpen   {0.333f, 0.871f, 0.522f};

const int   kF3RockCount      = 5;
const float kF3RockThrowRange = 8.0f;   // how far ahead of Roary a rock flies

const float kF3NoiseRadius   = 5.50f;

const float kF3GuardAlertTime = 4.00f;

const glm::vec3 kF3ConeAlerted {0.949f, 0.639f, 0.212f};

struct F3PatrolPoint {
    glm::vec3 position;
    float     pauseSeconds;
};

// The guard walks straight between points, so no line between neighbours may
// cross a wall. Put a point in each doorway he goes through.
const F3PatrolPoint kF3PatrolPoints[] = {
    { { -1.0f, 0.0f, -7.0f }, 1.2f },
    { {  6.0f, 0.0f, -5.0f }, 0.0f },
    { { 10.5f, 0.0f, -7.5f }, 1.2f },
    { { 10.5f, 0.0f, -1.5f }, 1.2f },
    { {  6.0f, 0.0f, -5.0f }, 0.0f },
    { {  0.0f, 0.0f,  5.0f }, 1.2f },
};

const int kF3PatrolCount =
    static_cast<int>(sizeof(kF3PatrolPoints) / sizeof(kF3PatrolPoints[0]));

const float kF3GuardSpeed = 2.60f;

const float     kF3CaughtFlashTime = 1.00f;  // how long Roary is tinted red after being caught
const glm::vec3 kF3CaughtTint {0.961f, 0.345f, 0.286f};

const glm::vec3 kF3CameraPos    {0.0f, 26.16f, 21.71f};
const glm::vec3 kF3CameraTarget {0.0f, 0.0f, 2.00f};
const float     kF3CameraFovDeg = 46.0f;

// Top floor menu colours
const glm::vec3 kF4Wall    {0.353f, 0.063f, 0.188f};
const glm::vec3 kF4CageBar {0.604f, 0.639f, 0.678f};

// Top floor - boss fight (BossScene)
const float kF4ArenaRadius   = 11.0f;
const float kF4DeckThickness = 0.30f;

const glm::vec3 kF4Deck     {0.180f, 0.055f, 0.098f};

const float kF4PlayerRadius = 0.42f;
const float kF4PlayerScale  = 1.55f;
const float kF4WalkSpeed    = 5.80f;
const glm::vec3 kF4StartPos {0.0f, 0.0f, 6.0f};

const float kF4DashSpeed    = 22.0f;
const float kF4DashDuration = 0.22f;
const float kF4DashCooldown = 5.00f;

const glm::vec3 kF4InvulnTint   {0.780f, 0.925f, 1.000f};
const float     kF4InvulnTintMix = 0.55f;

const int kF4MaxHearts = 3;

const int   kF4RockyHealth     = 3;
const float kF4RockyRadius     = 0.75f;
const float kF4RockyHeight     = 2.30f;
const float kF4RockyWalkSpeed  = 1.60f;

const glm::vec3 kF4RockyStartPos {0.0f, 0.0f, -4.0f};

const float kF4RockyKeepDistance = 4.0f;

const float kF4RockyWaitTime      = 1.10f;
const float kF4RockyTelegraphTime = 1.05f;
const float kF4RockyChargeSpeed   = 17.0f;
const float kF4RockyStunTime      = 3.20f;
const float kF4RockyRecoverTime   = 0.80f;

const float kF4RockyPunishRange = 1.70f;

const float kF4PlayerHitInvuln = 1.30f;

const glm::vec3 kF4RockyTelegraphTint {0.961f, 0.200f, 0.200f};
const glm::vec3 kF4RockyStunTint      {0.961f, 0.549f, 0.286f};

const float kF4VictoryTime = 2.60f;
const float kF4DefeatTime  = 2.20f;

const glm::vec3 kF4CameraPos    {0.0f, 27.44f, 21.71f};
const glm::vec3 kF4CameraTarget {0.0f, 0.0f, 2.50f};
const float     kF4CameraFovDeg = 46.0f;

//-----------------------------------------------------------------------------
// FLOOR METADATA  (name + one-line description shown on the menu)
//-----------------------------------------------------------------------------
struct FloorInfo {
    const char* name;         // shown large when the floor is selected
    const char* description;  // the one-line pitch under the name
    const char* shortLabel;   // small label drawn next to the floor itself
};

// The built-in font is uppercase-only, so these are written in caps.
const FloorInfo kFloorInfo[kFloorCount] = {
    { "FLOOR 1 - PLATFORM PERIL",
      "CLASSIC 3D PLATFORMING. RUN, JUMP AND CLIMB YOUR WAY TO THE STAIRS.",
      "PLATFORMER" },

    { "FLOOR 2 - THE SHIFTING TUNNEL",
      "SPRINT THROUGH A TUNNEL WHERE GRAVITY AND ORIENTATION KEEP FLIPPING.",
      "TUNNEL RUN" },

    { "FLOOR 3 - SILENT PAWS",
      "SNEAK PAST THE SEARCHLIGHTS AND GUARDS WITHOUT BEING SPOTTED.",
      "STEALTH" },

    { "TOP FLOOR - RIVAL SHOWDOWN",
      "BEAT THE OWL, THE IBIS AND THE BULL TO SET PAWLINA FREE.",
      "BOSS FIGHT" },
};

//-----------------------------------------------------------------------------
// FLOOR 1 - PLATFORMER TUNING
//
// Jump feel is driven by two numbers a designer can actually picture: how high
// the jump goes, and how long it takes to get there. Gravity and launch speed
// are DERIVED from those, so you never tune a gravity constant by trial and
// error - change kJumpHeight and the jump still reaches exactly that height.
//-----------------------------------------------------------------------------
namespace plat {

const float kJumpHeight = 2.30f;  // world units cleared by a full jump
const float kTimeToApex = 0.36f;  // seconds to reach the top

// Standard projectile motion solved for gravity and launch speed.
inline float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }

// Falling under heavier gravity than rising is the single cheapest way to stop
// a jump feeling floaty.
const float kFallGravityMultiplier = 1.85f;

// Releasing the jump button while still rising cuts the climb short, which is
// what gives you both a tap-hop and a full jump from one button.
const float kJumpCutMultiplier = 0.45f;

// Two forgiveness windows. Coyote time lets you jump just after walking off an
// edge; the buffer lets a press land slightly before you touch down and still
// fire. Without them the controls feel broken even when they are correct.
const float kCoyoteTime     = 0.12f;
const float kJumpBufferTime = 0.12f;

// Horizontal movement. Separate rates for speeding up, slowing down and
// turning around, and weaker versions of each in the air.
const float kMaxRunSpeed  = 6.4f;
const float kGroundAccel  = 60.0f;
const float kGroundDecel  = 55.0f;
const float kGroundTurn   = 95.0f;
const float kAirAccel     = 34.0f;
const float kAirDecel     = 12.0f;
const float kAirTurn      = 50.0f;

// Roary's collision box. Slightly narrower than he looks, which makes landing
// on the edge of a platform feel generous rather than fussy.
const glm::vec3 kPlayerSize {0.70f, 1.30f, 0.70f};

// Camera: a fixed angle that follows, rather than one the player rotates.
const glm::vec3 kCameraOffset {0.0f, 6.2f, 15.5f};
const glm::vec3 kCameraLookAt {0.0f, 1.1f, 0.0f};
const float     kCameraFovDeg = 52.0f;
const float     kCameraFollow = 6.5f;   // higher = tighter follow

// Level colors.
const glm::vec3 kPlatformWarm = kF1BlockWarm;
const glm::vec3 kPlatformCool = kF1BlockCool;
const glm::vec3 kPlatformEdge {0.180f, 0.220f, 0.290f}; // side faces
const glm::vec3 kMovingPlatform {0.976f, 0.729f, 0.247f}; // #F9BA3F
const glm::vec3 kGoalColor      {0.345f, 0.957f, 0.667f}; // #58F4AA
const glm::vec3 kShadowColor    {0.020f, 0.040f, 0.080f};

// --- enemies and stomping ---------------------------------------------------
// The stomp is the mechanic the whole genre is built on: land on something and
// it dies, and you bounce. Getting the bounce right matters - too little and
// it feels like a stumble, too much and you lose control.
const float kStompBounceSpeed = 10.5f;

// How far into the enemy's top you must be for it to count as a stomp rather
// than a side hit. Generous on purpose: a near miss reading as a death is the
// most infuriating thing a platformer can do.
const float kStompGrace = 0.40f;

const int   kStartingLives    = 3;
const float kInvulnerableTime = 1.60f;  // after taking a hit
const float kHitKnockback     = 7.0f;
const float kHitKnockUp       = 7.5f;
const float kEnemySquashTime  = 0.32f;  // how long the squash shows

const glm::vec3 kEnemySize      {0.78f, 0.72f, 0.78f};
const glm::vec3 kWalkerBody     {0.682f, 0.286f, 0.447f}; // #AE4972
const glm::vec3 kWalkerFoot     {0.400f, 0.145f, 0.255f}; // #662541
const glm::vec3 kSpikerBody     {0.306f, 0.275f, 0.420f}; // #4E466B
const glm::vec3 kSpikerSpike    {0.878f, 0.906f, 0.937f}; // pale spikes

// --- item blocks ------------------------------------------------------------
// Solid from every side, but punching the underside pops a coin out. The used
// block dims so you can see at a glance which are spent.
const glm::vec3 kItemBlockSize  {1.20f, 1.20f, 1.20f};
const glm::vec3 kItemBlockLive  {0.961f, 0.690f, 0.196f}; // #F5B032
const glm::vec3 kItemBlockTrim  {0.996f, 0.886f, 0.553f};
const glm::vec3 kItemBlockSpent {0.325f, 0.306f, 0.282f}; // #534E48
const float     kPoppedCoinTime = 0.70f;
const float     kPoppedCoinRise = 1.90f;

// --- bounce pads ------------------------------------------------------------
const float     kBouncePadSpeed = 17.0f;  // reaches ~4.1 units up
const glm::vec3 kBouncePadColor {0.400f, 0.878f, 0.945f}; // #66E0F1
const float     kBouncePadSquash = 0.22f; // animation length

const float kCoinRadius   = 0.40f;  // how close counts as collected
const float kCoinSpinRate = 110.0f; // degrees per second
const float kGoalHoldTime = 1.40f;  // celebration before returning

} // namespace plat

//-----------------------------------------------------------------------------
// FLOOR 2 - SHIFTING TUNNEL TUNING
//
// You auto-run down a square tunnel. Run off the edge of the surface you are
// on and you transition onto the neighbouring wall, which becomes the new
// floor - the world does not move, your sense of down rotates.
//
// The orientation is a DISCRETE state (which of the four faces is down), never
// an integrated rotation. Only the camera's roll angle interpolates. Faking
// this with forces and free rotation is the documented way to get drift, and
// snapping to four known states cannot drift by construction.
//-----------------------------------------------------------------------------
namespace tunnel {

const float kTunnelSide    = 6.0f;   // the square cross-section
const int   kLanesPerFace  = 3;
const float kSegmentLength = 2.4f;   // depth of one row of tiles
const float kTileThickness = 0.45f;

inline float laneWidth() { return kTunnelSide / static_cast<float>(kLanesPerFace); }
inline float halfSide()  { return kTunnelSide * 0.5f; }

// Auto-run. The speed creeps up so the level gets harder without new content.
const float kStartSpeed = 11.0f;
const float kMaxSpeed   = 16.5f;
const float kSpeedRamp  = 0.22f;   // units/sec gained per second

// Strafing across the face you are on.
const float kStrafeSpeed = 9.0f;
const float kStrafeAccel = 70.0f;

// Jump, derived the same way as the platformer: pick the height and the time
// to reach it, and gravity follows.
const float kJumpHeight = 2.20f;
const float kTimeToApex = 0.30f;
inline float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }
const float kFallGravityMultiplier = 1.70f;
const float kCoyoteTime = 0.10f;

// Falling this far past the surface means you missed, and the run restarts.
const float kFallLimit = 7.0f;

// How fast the view rolls when you change face. Quick enough to keep up, slow
// enough that you can see which way the world turned.
const float kRollSpeed = 420.0f;     // degrees per second

// Camera sits behind and above, measured in the CURRENT face's frame so it
// rolls with you.
const float kCameraBack   = 7.5f;
const float kCameraUp     = 2.6f;
const float kCameraAhead  = 6.0f;
const float kCameraFovDeg = 62.0f;

// Colors. The tunnel is the dark floor of the tower, lit by its own edges.
const glm::vec3 kTileTopA   {0.541f, 0.333f, 0.776f}; // #8A55C6 violet
const glm::vec3 kTileTopB   {0.345f, 0.776f, 0.871f}; // #58C6DE cyan
const glm::vec3 kTileBody   {0.094f, 0.063f, 0.157f}; // #180F28
const glm::vec3 kVoidColor  {0.020f, 0.012f, 0.039f};
const glm::vec3 kGoalRing   {0.400f, 0.976f, 0.706f};

// How many segments ahead are drawn. Beyond this the tunnel fades into dark.
const int kDrawDistance = 26;

const glm::vec3 kPlayerSize {0.80f, 1.30f, 0.80f};

} // namespace tunnel

} // namespace cfg
