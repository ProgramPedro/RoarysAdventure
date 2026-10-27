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
inline constexpr int  kWindowWidth   = 1280;
inline constexpr int  kWindowHeight  = 800;
inline constexpr char kWindowTitle[] = "Roary's Adventure: Save Pawlina!";

//-----------------------------------------------------------------------------
// FIU-THEMED BACKGROUND  (blue and gold - the school colors)
//
// The background and the on-screen text are the FIU-branded part of the game.
// The tower itself deliberately uses its OWN palette further down so it reads
// as a separate object instead of blending into the backdrop.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kBgTop     {0.031f, 0.118f, 0.247f}; // #081E3F FIU Blue
inline constexpr glm::vec3 kBgBottom  {0.008f, 0.035f, 0.086f}; // #020916 deeper navy
inline constexpr glm::vec3 kBgGlow    {0.714f, 0.525f, 0.173f}; // #B6862C FIU Gold
inline constexpr glm::vec2 kBgGlowPos {0.0f, 0.35f};            // in NDC (-1..1)
inline constexpr float     kBgGlowRadius   = 1.15f;
inline constexpr float     kBgGlowStrength = 0.55f;

// FIU gold / blue reused by the UI text and by Roary himself.
inline constexpr glm::vec3 kFiuGold {0.839f, 0.682f, 0.216f};  // #D6AE37 brighter gold
inline constexpr glm::vec3 kFiuBlue {0.090f, 0.251f, 0.494f};  // #17407E lighter blue

//-----------------------------------------------------------------------------
// UI TEXT
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kTextPrimary   {0.960f, 0.960f, 0.980f}; // near-white
inline constexpr glm::vec3 kTextDim       {0.600f, 0.640f, 0.720f}; // hints
inline constexpr glm::vec3 kTextLocked    {0.820f, 0.380f, 0.380f}; // LOCKED label
inline constexpr glm::vec3 kTextComplete  {0.380f, 0.880f, 0.560f}; // COMPLETE label

// Text sizes are "how many screen pixels per font pixel" (the font is 5x7).
inline constexpr float kTitleScale       = 7.0f;
inline constexpr float kSubtitleScale    = 4.0f;
inline constexpr float kFloorNameScale   = 4.0f;
inline constexpr float kDescriptionScale = 2.0f;
inline constexpr float kHintScale        = 2.0f;
inline constexpr float kFloorLabelScale  = 2.0f;

//-----------------------------------------------------------------------------
// TOWER PALETTE  (intentionally NOT FIU blue/gold)
//
// The tower is weathered stone with a per-floor accent color, so each floor
// hints at the game it is based on.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kTowerStone     {0.290f, 0.306f, 0.341f}; // #4A4E57 slate
inline constexpr glm::vec3 kTowerStoneDark {0.192f, 0.204f, 0.235f}; // #31343C shadow
inline constexpr glm::vec3 kTowerBase      {0.235f, 0.212f, 0.192f}; // #3C3631 foundation
inline constexpr glm::vec3 kTowerMerlon    {0.482f, 0.506f, 0.553f}; // #7B818D battlements

// Locked floors are faded toward this color by kLockedFade (0 = normal, 1 = fully faded).
inline constexpr glm::vec3 kLockedGrey = {0.180f, 0.188f, 0.204f};
inline constexpr float     kLockedFade = 0.60f;

// The pulsing frame drawn around the currently selected floor.
inline constexpr glm::vec3 kSelectFrame      {1.000f, 0.960f, 0.870f}; // warm white
inline constexpr float     kSelectPulseSpeed = 3.2f;
inline constexpr float     kSelectFrameThick = 0.12f;

//-----------------------------------------------------------------------------
// TOWER GEOMETRY
//
// Floors are stacked along +Y. Floor 0 is the bottom floor, floor 3 is the top.
//-----------------------------------------------------------------------------
inline constexpr int   kFloorCount   = 4;
inline constexpr float kFloorWidth   = 7.0f;   // full width of the tower
inline constexpr float kFloorHeight  = 2.30f;  // Y size of one floor
inline constexpr float kFloorGap     = 0.12f;  // visible seam between floors
inline constexpr float kTowerBaseY   = 0.0f;   // Y of the bottom of floor 0

// The tower is a ROUND stack of drum-shaped floors, so its horizontal size is
// a radius rather than a width and a depth.
inline constexpr float kTowerRadius  = kFloorWidth * 0.5f;

// Y coordinate of the CENTER of a given floor.
inline constexpr float floorCenterY(int floorIndex) {
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
inline constexpr float kSlabThickness = 0.22f;  // floor / ceiling discs
inline constexpr float kPillarWidth   = 0.42f;  // columns framing the opening
inline constexpr float kWallThickness = 0.45f;  // thickness of the curved wall
inline constexpr float kPropZ         = 0.0f;   // props sit on the tower axis

// The curved wall is drawn as a ring of flat segments. More segments looks
// rounder; 18 across the covered arc is plenty at this size.
inline constexpr int   kWallSegments  = 18;

// How much of the circle is left open at the front so you can see inside,
// centered on +Z (toward the camera).
inline constexpr float kWallOpeningDegrees = 112.0f;

// Discs stick out past the wall a little, giving each floor a lip.
inline constexpr float kSlabOverhang = 0.28f;

//-----------------------------------------------------------------------------
// THE OPEN CASTLE TOP
//
// The highest floor has no ceiling. Instead it is ringed by a low parapet with
// battlements, so the boss fight happens on an open roof under the sky.
//-----------------------------------------------------------------------------
// Sized so the merlons and the gaps between them are about equal, which is
// what makes a battlement read as a battlement instead of a bumpy rim.
inline constexpr float kParapetHeight = 0.55f;  // the solid low wall
inline constexpr float kMerlonHeight  = 0.62f;  // the blocks on top of it
inline constexpr float kMerlonWidth   = 0.78f;
inline constexpr int   kMerlonCount   = 10;     // spaced around the covered arc

// The boss arena stands on a low round dais. Besides looking like a proper
// arena, it lifts the figures clear of the deck's rim, which would otherwise
// hide their legs from a camera looking slightly up at the tower.
inline constexpr float kBossDaisHeight = 0.40f;
inline constexpr float kBossDaisRadius = 3.00f;

// How tall the top floor COUNTS as for the selection ring and mouse picking.
// Its room is open, so using the full floor height would make you able to
// select it by pointing at empty sky.
inline constexpr float kTopFloorPickHeight = 1.90f;

//-----------------------------------------------------------------------------
// UI SCALING
//
// All UI positions and text sizes in the scenes are written in "design pixels"
// against this reference window height, then multiplied by
// (framebufferHeight / kUiReferenceHeight). That keeps the interface the same
// apparent size on a normal display and on a Retina display, where the
// framebuffer is twice the window size.
//-----------------------------------------------------------------------------
inline constexpr float kUiReferenceHeight = 800.0f;

//-----------------------------------------------------------------------------
// CAMERA  (level-select screen)
//
// A gentle three-quarter side view: mostly side-on so the tower reads as a
// stack of floors, but angled just enough to show depth.
//-----------------------------------------------------------------------------
// Framed so the tower spans about 55% of the window height, sitting just
// below the title and just above the info panel. The target sits a little
// BELOW the tower's center, which pushes the tower slightly up the screen.
inline constexpr glm::vec3 kMenuCameraPos    {5.8f, 4.9f, 25.8f};
inline constexpr glm::vec3 kMenuCameraTarget {-0.2f, 2.62f, 0.0f};
inline constexpr float     kMenuCameraFovDeg = 42.0f;
inline constexpr float     kCameraNear       = 0.1f;
inline constexpr float     kCameraFar        = 100.0f;

// Very subtle idle drift so the menu is not perfectly static. Set to 0 to stop.
inline constexpr float kCameraSwayAmount = 0.30f;
inline constexpr float kCameraSwaySpeed  = 0.35f;

// Direction the single scene light travels (it is normalized at use).
inline constexpr glm::vec3 kLightDirection {-0.45f, -0.80f, -0.40f};

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
inline constexpr glm::vec3 kPantherFur   {0.784f, 0.576f, 0.333f}; // #C89355
inline constexpr glm::vec3 kPantherCream {0.910f, 0.824f, 0.682f}; // #E8D2AE muzzle
inline constexpr glm::vec3 kPantherInner {0.549f, 0.369f, 0.200f}; // #8C5E33 inner ear
inline constexpr glm::vec3 kPantherNose  {0.227f, 0.165f, 0.133f}; // #3A2A22
inline constexpr glm::vec3 kEyeDark      {0.102f, 0.094f, 0.110f}; // #1A181C pupils
inline constexpr glm::vec3 kEyeWhite     {0.965f, 0.961f, 0.953f};

// The FIU kit both panthers wear.
inline constexpr glm::vec3 kFiuKit       {0.086f, 0.145f, 0.353f}; // #16255A navy
inline constexpr glm::vec3 kFiuKitTrim   {0.950f, 0.950f, 0.960f}; // white lettering
inline constexpr glm::vec3 kPawlinaTrim  {0.788f, 0.635f, 0.153f}; // #C9A227 gold hem

// FAU's Owlsley.
inline constexpr glm::vec3 kOwlBody   {0.290f, 0.204f, 0.149f}; // #4A3426 dark brown
inline constexpr glm::vec3 kOwlCream  {0.847f, 0.773f, 0.659f}; // #D8C5A8 beak, speckles
inline constexpr glm::vec3 kOwlBrow   {0.949f, 0.361f, 0.098f}; // #F25C19 signature orange
inline constexpr glm::vec3 kOwlEye    {0.180f, 0.545f, 0.831f}; // #2E8BD4 blue
inline constexpr glm::vec3 kOwlJersey {0.169f, 0.333f, 0.722f}; // #2B55B8 FAU blue, lifted
inline constexpr glm::vec3 kOwlPants  {0.925f, 0.925f, 0.937f}; // white

// Miami's Sebastian.
inline constexpr glm::vec3 kIbisBody   {0.949f, 0.941f, 0.918f}; // #F2F0EA white
inline constexpr glm::vec3 kIbisBill   {0.910f, 0.384f, 0.165f}; // #E8622A orange
inline constexpr glm::vec3 kIbisJersey {0.957f, 0.451f, 0.129f}; // #F47321 Miami orange
inline constexpr glm::vec3 kIbisFeet   {0.949f, 0.761f, 0.188f}; // #F2C230 yellow
inline constexpr glm::vec3 kIbisGreen  {0.000f, 0.314f, 0.188f}; // #005030 hat band

// USF's Rocky.
inline constexpr glm::vec3 kBullBody   {0.608f, 0.608f, 0.588f}; // #9B9B96 grey
inline constexpr glm::vec3 kBullMuzzle {0.749f, 0.741f, 0.714f}; // #BFBDB6
inline constexpr glm::vec3 kBullHorn   {0.831f, 0.686f, 0.216f}; // #D4AF37 gold
inline constexpr glm::vec3 kBullJersey {0.043f, 0.478f, 0.310f}; // #0B7A4F USF green, lifted
inline constexpr glm::vec3 kBullPants  {0.812f, 0.769f, 0.576f}; // #CFC493 vegas gold
inline constexpr glm::vec3 kBullHoof   {0.129f, 0.125f, 0.133f};

//-----------------------------------------------------------------------------
// ROARY MARKER  (the panther who rides the lift up and down the tower)
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kRoaryAccent = kFiuGold;  // his lift platform
inline constexpr glm::vec3 kRoarySize   {1.00f, 1.42f, 1.00f};

// Roary stands to the LEFT of the tower and slides up/down to the selection.
inline constexpr float kRoaryOffsetX   = -kTowerRadius - 1.25f;
inline constexpr float kRoaryOffsetZ   = 1.1f;
inline constexpr float kRoaryMoveSpeed = 8.0f;   // higher = snappier slide
inline constexpr float kRoaryBobAmount = 0.09f;
inline constexpr float kRoaryBobSpeed  = 2.6f;

//-----------------------------------------------------------------------------
// PER-FLOOR THEME COLORS
//-----------------------------------------------------------------------------
// Floor 1 - Mario-style platformer: bright, cheerful blocks.
inline constexpr glm::vec3 kF1Wall      {0.333f, 0.447f, 0.600f}; // #556F99 bright blue-grey
inline constexpr glm::vec3 kF1BlockWarm {1.000f, 0.478f, 0.302f}; // #FF7A4D coral
inline constexpr glm::vec3 kF1BlockCool {0.310f, 0.878f, 0.627f}; // #4FE0A0 mint
inline constexpr glm::vec3 kF1Coin      {1.000f, 0.835f, 0.310f}; // #FFD54F coin yellow

// Floor 2 - "Run"-style shifting tunnel: near-black with a violet rim.
inline constexpr glm::vec3 kF2Wall    {0.078f, 0.047f, 0.118f}; // #140C1F near black
inline constexpr glm::vec3 kF2Tunnel  {0.027f, 0.016f, 0.047f}; // #07040C void
inline constexpr glm::vec3 kF2RimGlow {0.420f, 0.247f, 0.627f}; // #6B3FA0 violet
inline constexpr glm::vec3 kF2Accent  {0.541f, 0.886f, 0.937f}; // #8AE2EF cyan edge

// Floor 3 - Stealth: dark teal room, pale searchlights, black guards.
inline constexpr glm::vec3 kF3Wall        {0.082f, 0.200f, 0.227f}; // #15333A dark teal
inline constexpr glm::vec3 kF3Light       {0.812f, 0.965f, 1.000f}; // #CFF6FF searchlight
inline constexpr glm::vec3 kF3Guard       {0.047f, 0.059f, 0.071f}; // #0C0F12 silhouette
inline constexpr float     kF3LightAlpha  = 0.34f;
inline constexpr float     kF3SweepSpeed  = 0.9f;
inline constexpr float     kF3SweepDeg    = 26.0f;  // half-angle of the sweep

// Top floor - Boss fight: crimson room, caged Pawlina, three rival mascots.
inline constexpr glm::vec3 kF4Wall    {0.353f, 0.063f, 0.188f}; // #5A1030 deep crimson
inline constexpr glm::vec3 kF4CageBar {0.604f, 0.639f, 0.678f}; // #9AA3AD silver


//-----------------------------------------------------------------------------
// FLOOR METADATA  (name + one-line description shown on the menu)
//-----------------------------------------------------------------------------
struct FloorInfo {
    const char* name;         // shown large when the floor is selected
    const char* description;  // the one-line pitch under the name
    const char* shortLabel;   // small label drawn next to the floor itself
};

// The built-in font is uppercase-only, so these are written in caps.
inline constexpr FloorInfo kFloorInfo[kFloorCount] = {
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
// ASSET PATHS  (unused for now - every object is a placeholder primitive)
//
// When real art is ready, load these in the scene that needs them. Paths are
// relative to the executable; CMake copies assets/ next to the binary.
//-----------------------------------------------------------------------------
inline constexpr char kAssetRoary[]   = "assets/roary.png";
inline constexpr char kAssetPawlina[] = "assets/pawlina.png";
inline constexpr char kAssetStone[]   = "assets/stone.png";

} // namespace cfg
