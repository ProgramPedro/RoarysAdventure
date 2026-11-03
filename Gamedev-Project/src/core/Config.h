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

//-----------------------------------------------------------------------------
// FLOOR 3 GAMEPLAY  (the stealth level itself, not the menu diorama above)
//
// The room lies on the XZ plane with +Y up, centered on the origin, so the
// walls sit at +/-kF3RoomHalfX and +/-kF3RoomHalfZ and every position below
// reads as an offset from the middle of the room. Roary never leaves y = 0:
// he does not jump, so the level is really a 2D problem wearing a 3D coat.
//
// On screen, with the fixed camera further down: -Z is the FAR wall and +Z is
// the near one, so W moves away from the viewer and S moves toward them.
//-----------------------------------------------------------------------------
inline constexpr float kF3RoomHalfX  = 16.0f;  // half the floor's X size
inline constexpr float kF3RoomHalfZ  = 10.5f;  // half the floor's Z size
inline constexpr float kF3WallHeight = 1.80f;
inline constexpr float kF3WallThick  = 0.50f;

// The NEAR (+Z) wall is only a low lip. A full-height wall there would stand
// between the camera and the room and hide the front row entirely - the same
// problem the level-select tower solves by leaving its front open. Collision
// is unaffected: clampToRoom() does not care how tall a wall is drawn.
inline constexpr float kF3NearWallHeight = 0.34f;

// Interior walls are shorter than the perimeter. From an angled overhead
// camera a full-height divider hides whatever stands behind it, and in a
// stealth level the thing hidden is usually a guard you needed to see.
inline constexpr float kF3InnerWallHeight = 1.15f;

//-----------------------------------------------------------------------------
// THE ROOM LAYOUT
//
// Interior walls divide the floor into four rooms joined by doorways, so the
// level is a route to work out rather than one open box. Each entry is an
// axis-aligned slab given as a centre and a size on the XZ plane; they are
// what the collision in StealthScene actually tests against, so editing this
// table changes both what is drawn and where Roary can walk. Keep the gaps
// at least a couple of player diameters wide or a doorway becomes a wall.
//
//        -X                                                   +X
//    -Z   +-----------+-------------------+-----------------+
//         |           |                   |     ROOM D      |
//         |  ROOM A   |      ROOM B       |-----_____-------|   <- divider C
//         |      _    |          _        |     ROOM C      |
//    +Z   +------|----+----------|--------+-----------------+
//            divider A      divider B
//-----------------------------------------------------------------------------
struct F3WallSlab {
    float centerX;
    float centerZ;
    float sizeX;
    float sizeZ;
};

inline constexpr F3WallSlab kF3InnerWalls[] = {
    // Divider A, at x = -4, with a doorway at z = 1.0 .. 4.0.
    { -4.0f, -4.75f, kF3WallThick, 11.5f },
    { -4.0f,  7.25f, kF3WallThick,  6.5f },

    // Divider B, at x = +6, with a doorway at z = -6.5 .. -3.5.
    {  6.0f, -8.50f, kF3WallThick,  4.0f },
    {  6.0f,  3.50f, kF3WallThick, 14.0f },

    // Divider C, at z = 0, splitting the right-hand side into rooms C and D.
    // Doorway at x = 11.0 .. 14.0.
    {  8.50f, 0.0f,  5.0f, kF3WallThick },
    { 15.00f, 0.0f,  2.0f, kF3WallThick },
};

inline constexpr int kF3InnerWallCount =
    static_cast<int>(sizeof(kF3InnerWalls) / sizeof(kF3InnerWalls[0]));

// The deck is a checkerboard. In a game with no textures at all, that pattern
// is the only thing giving a sense of speed and distance as Roary walks, so it
// is doing real work rather than just decoration.
inline constexpr glm::vec3 kF3FloorA   {0.063f, 0.129f, 0.149f}; // #101F26 dark tile
inline constexpr glm::vec3 kF3FloorB   {0.086f, 0.169f, 0.192f}; // #162B31 light tile
inline constexpr glm::vec3 kF3WallIn   {0.110f, 0.235f, 0.267f}; // #1C3C44 inner wall
inline constexpr glm::vec3 kF3Trim     {0.169f, 0.357f, 0.396f}; // #2B5B65 top capping
inline constexpr float     kF3TileSize = 2.50f;

// Roary. kF3PlayerRadius is what the wall collision actually uses, so it is
// his real size rather than just how big he looks.
inline constexpr float kF3PlayerRadius = 0.42f;
inline constexpr float kF3PlayerScale  = 1.55f;  // multiplies his natural 1.42 height
inline constexpr float kF3WalkSpeed    = 5.20f;  // world units per second
inline constexpr float kF3SneakSpeed   = 2.30f;  // while Shift is held
inline constexpr float kF3TurnSpeed    = 11.0f;  // how fast he swings to face his heading

// Where he starts: the near-left corner, diagonally opposite the far wall the
// exit door will eventually sit in.
inline constexpr glm::vec3 kF3StartPos {-kF3RoomHalfX + 2.0f, 0.0f, kF3RoomHalfZ - 2.0f};

// A soft blob shadow, used under Roary and the guards alike. Without one they
// read as hovering, because the scene has a single directional light and no
// shadow mapping.
inline constexpr glm::vec3 kF3ShadowColor {0.020f, 0.047f, 0.055f};
inline constexpr float     kF3ShadowAlpha = 0.55f;

//-----------------------------------------------------------------------------
// GUARDS
//
// For now a guard is a block that stands still. Patrol routes, vision cones
// and detection come later; what matters here is that he is placed in the same
// world the collision uses and is easy to see against the dark deck.
//
// kF3Guard further up is the near-black silhouette the level-select diorama
// draws. It is too close to the deck colour to read during play, so the guards
// in the level itself use the lighter body below with a bright visor. The
// visor also shows which way a guard is facing, which is what the vision cone
// will be built on.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kF3GuardBody  {0.188f, 0.208f, 0.247f}; // #30353F slate
inline constexpr glm::vec3 kF3GuardHead  {0.251f, 0.275f, 0.318f}; // #404651 lighter
inline constexpr glm::vec3 kF3GuardVisor {0.949f, 0.639f, 0.212f}; // #F2A336 amber

inline constexpr float kF3GuardRadius     = 0.45f;  // for collision, later
inline constexpr float kF3GuardBodyHeight = 1.45f;
inline constexpr float kF3GuardHeadSize   = 0.52f;

// Standing in the middle room, facing the near wall. One guard for now; this
// becomes a table once there are patrol routes to give them.
inline constexpr glm::vec3 kF3GuardStart   {1.0f, 0.0f, -2.0f};

// Degrees about Y, using the same convention as Roary's facing: 0 looks down
// +Z (toward the camera), 90 looks down +X, 180 looks away from the camera.
// Angled rather than square-on so he reads as a solid figure, not a flat tile.
inline constexpr float     kF3GuardFacing  = 35.0f;

//-----------------------------------------------------------------------------
// GUARD VISION
//
// A guard sees Roary when all three of these hold: he is within range, he is
// inside the cone's half-angle, and no wall stands between them. The checks
// run in that order because each is more expensive than the last.
//
// The cone is drawn as a flat wedge lying on the deck rather than a beam from
// the guard's eyes. In a top-down stealth level the question the player is
// always asking is "which floor is dangerous", and a shape on the floor
// answers it directly.
//-----------------------------------------------------------------------------
inline constexpr float kF3VisionRange        = 9.0f;   // world units
inline constexpr float kF3VisionHalfAngleDeg = 30.0f;  // half the cone's spread

// Where the wedge sits and how thick it is. Low and thin, so it reads as light
// on the floor instead of a solid block in the way.
inline constexpr float kF3ConeApexY  = 0.26f;
inline constexpr float kF3ConeHeight = 0.40f;

inline constexpr glm::vec3 kF3ConeCalm  {0.675f, 0.886f, 0.937f}; // #ACE2EF pale cyan
inline constexpr glm::vec3 kF3ConeAlert {0.961f, 0.345f, 0.286f}; // #F55849 alarm red
inline constexpr float     kF3ConeAlpha = 0.20f;

//-----------------------------------------------------------------------------
// KEY ITEMS
//
// The things Roary has to collect before the exit door will open. Drawn as
// paw prints for now. One sits in each of the four rooms, so finishing the
// level means crossing the whole floor rather than slipping along one wall.
//
// They are picked up by walking near them - no key to press. A stealth level
// already asks enough of the player's hands, and a pickup you can miss by
// standing in the right place and not pressing anything is just annoying.
//-----------------------------------------------------------------------------
inline constexpr glm::vec3 kF3ItemPositions[] = {
    { -12.5f, 0.0f, -7.0f },  // room A, far corner from the start
    {  -1.5f, 0.0f,  7.0f },  // room B, past the guard
    {  11.5f, 0.0f,  6.0f },  // room C
    {  12.0f, 0.0f, -6.5f },  // room D
};

inline constexpr int kF3ItemCount =
    static_cast<int>(sizeof(kF3ItemPositions) / sizeof(kF3ItemPositions[0]));

// Generous on purpose: walking "onto" a thing the player can see should be
// enough. Measured from the item's centre to Roary's.
inline constexpr float kF3ItemPickupRadius = 1.10f;

// Paw colours. FIU gold against the dark teal deck is the strongest contrast
// in the palette, which is what a collectable wants.
inline constexpr glm::vec3 kF3ItemPaw   = kFiuGold;
inline constexpr glm::vec3 kF3ItemGlow  {0.980f, 0.843f, 0.420f};  // #FAD76B halo

// Hovering and turning, so they read as "pick me up" rather than as scenery.
inline constexpr float kF3ItemHoverY     = 0.80f;
inline constexpr float kF3ItemBobAmount  = 0.14f;
inline constexpr float kF3ItemBobSpeed   = 2.10f;
inline constexpr float kF3ItemSpinSpeed  = 48.0f;  // degrees per second
inline constexpr float kF3ItemScale      = 1.65f;

//-----------------------------------------------------------------------------
// THE EXIT DOOR
//
// Set into the RIGHT (+X) wall near the +Z end - the bottom right of the
// screen - so it is diagonally opposite where Roary starts and reaching it
// means having crossed the floor. It opens only once every paw has been
// collected, and walking into it is what finishes the level.
//
// The door is a TRIGGER, not a hole: Roary is still stopped by the outer wall
// at +kF3RoomHalfX. Stepping into the zone in front of the door is what counts
// as going through it, which avoids having to special-case the clamp.
//-----------------------------------------------------------------------------
inline constexpr float kF3DoorZ      = 6.00f;  // centre along the right wall
inline constexpr float kF3DoorWidth  = 2.60f;  // how much of the wall it fills
inline constexpr float kF3DoorHeight = 1.45f;

// How close Roary has to get. Comfortably larger than his radius, so brushing
// the door counts rather than demanding he line up exactly.
inline constexpr float kF3DoorTriggerRadius = 1.80f;

inline constexpr glm::vec3 kF3DoorLocked {0.451f, 0.149f, 0.173f}; // #73262C dull red
inline constexpr glm::vec3 kF3DoorOpen   {0.333f, 0.871f, 0.522f}; // #55DE85 green
inline constexpr glm::vec3 kF3DoorFrame  {0.310f, 0.467f, 0.502f}; // #4F7780 stone frame

// How long the LOCKED message flashes after walking into a door you cannot
// open yet. Long enough to read, short enough not to nag.
inline constexpr float kF3LockedFlashTime = 1.60f;

//-----------------------------------------------------------------------------
// THROWN ROCKS
//
// Press E to aim, move the mouse to pick a spot on the floor, click to throw.
// The rock flies an arc to where you pointed - unless a wall is in the way, in
// which case it stops dead at the wall and the noise happens THERE instead.
// That difference is the whole tactic: a rock bounced off the near side of a
// divider pulls a guard toward you, one lobbed through a doorway pulls him
// away.
//
// The noise is drawn rather than heard: this project has no audio library, so
// an expanding ring on the deck is the cue.
//-----------------------------------------------------------------------------
inline constexpr int   kF3RockCount      = 5;      // how many Roary carries
inline constexpr float kF3RockThrowRange = 12.0f;  // furthest he can lob one
inline constexpr float kF3RockSpeed      = 15.0f;  // horizontal units per second
inline constexpr float kF3RockArcHeight  = 1.90f;  // peak of the arc
inline constexpr float kF3RockRadius     = 0.17f;

inline constexpr glm::vec3 kF3RockColor {0.545f, 0.580f, 0.620f}; // #8B949E grey

// The aiming marker. Two colours, because where the rock LANDS is the thing
// the player is choosing, and it is not always where they are pointing.
inline constexpr glm::vec3 kF3MarkerOpen    {0.675f, 0.886f, 0.937f}; // clear path
inline constexpr glm::vec3 kF3MarkerBlocked {0.949f, 0.639f, 0.212f}; // stops at a wall

// The noise a landed rock makes: a ring that expands to this radius and fades.
// Guards do not react to it yet - that arrives with patrol routes - but the
// radius is already the number their hearing check will use.
inline constexpr float kF3NoiseRadius   = 5.50f;
inline constexpr float kF3NoiseDuration = 1.30f;
inline constexpr glm::vec3 kF3NoiseColor {0.812f, 0.965f, 1.000f};

//-----------------------------------------------------------------------------
// GUARD DISTRACTION
//
// A rock landing within kF3NoiseRadius of a guard pulls his attention: he
// turns in place toward the sound, an exclamation mark appears over his head,
// and after kF3GuardAlertTime he turns back and carries on. Since nothing
// patrols yet, "carries on" means returning to the facing he started with and
// staying exactly where he is.
//
// Hearing deliberately IGNORES walls. Sound goes around corners, and a rock
// thrown through a doorway to pull a guard off a room he is watching is the
// whole point of carrying rocks - a wall-blocked hearing test would kill it.
//-----------------------------------------------------------------------------
inline constexpr float kF3GuardAlertTime     = 4.00f;  // seconds the ! stays up
inline constexpr float kF3GuardAlertTurnRate = 5.00f;  // how fast he snaps round

// The exclamation mark. Bright and emissive so it carries across the room,
// since it is the only confirmation the player gets that a throw worked.
inline constexpr glm::vec3 kF3AlertMark      {1.000f, 0.776f, 0.180f}; // #FFC62E
// Generous clearance: the camera looks down at about 53 degrees, so anything
// directly above the guard projects BEHIND him on screen and a mark held close
// to his head is swallowed by his own silhouette.
inline constexpr float     kF3AlertMarkY     = 1.15f;
inline constexpr float     kF3AlertBobSpeed  = 7.00f;
inline constexpr float     kF3AlertBobAmount = 0.07f;

// The cone while he is distracted but has not actually seen anybody: amber,
// between the calm cyan and the alarm red.
inline constexpr glm::vec3 kF3ConeAlerted {0.949f, 0.639f, 0.212f}; // #F2A336

//-----------------------------------------------------------------------------
// GUARD PATROL AND NAVIGATION
//
// The guard walks a loop of patrol points. He does NOT walk at them in a
// straight line: the floor has walls and doorways in the way, so the route
// between two points is worked out with Dijkstra over a grid of the walkable
// deck, built once when the level starts.
//
// kF3NavCell is the resolution of that grid. Smaller is a tighter fit through
// doorways and a slower search; 0.5 gives about 2,700 cells here, which solves
// in well under a millisecond and only ever runs when he picks a new target.
//-----------------------------------------------------------------------------
inline constexpr float kF3NavCell = 0.50f;

// The loop. Point 1 is in room D, so getting there means routing through the
// doorway in divider B rather than walking at the wall - which is the whole
// reason there is a pathfinder.
inline constexpr glm::vec3 kF3PatrolPoints[] = {
    {  -1.0f, 0.0f, -7.0f },  // room B, far side
    {  10.5f, 0.0f, -7.5f },  // room D, through divider B's doorway
    {  10.5f, 0.0f, -1.5f },  // room D, up against divider C
    {   0.0f, 0.0f,  5.0f },  // room B, near side
};

inline constexpr int kF3PatrolCount =
    static_cast<int>(sizeof(kF3PatrolPoints) / sizeof(kF3PatrolPoints[0]));

inline constexpr float kF3GuardSpeed      = 2.60f;  // slower than Roary walks
inline constexpr float kF3GuardTurnRate   = 7.00f;
inline constexpr float kF3GuardPauseTime  = 1.20f;  // a look around at each point
inline constexpr float kF3GuardArriveDist = 0.20f;  // "close enough" to a node

//-----------------------------------------------------------------------------
// SUSPICION
//
// Being seen does not catch you instantly. A meter fills while the guard has
// eyes on Roary and drains once he does not, and only a full meter fails the
// level. Instant detection reads as unfair and is miserable to play - the
// meter is what gives you the "get back behind the wall" moment that makes
// stealth fun.
//
// It fills faster the closer he is, so being spotted across the room is a
// warning and being spotted at his feet is not.
//-----------------------------------------------------------------------------
inline constexpr float kF3SuspicionNearRate = 1.45f;  // per second, point blank
inline constexpr float kF3SuspicionFarRate  = 0.38f;  // per second, at max range
inline constexpr float kF3SuspicionSneakMul = 0.62f;  // sneaking is less visible
inline constexpr float kF3SuspicionDrain    = 0.42f;  // per second, out of sight
inline constexpr float kF3SuspicionGrace    = 0.80f;  // hold before it drains
inline constexpr float kF3CaughtFlashTime   = 2.20f;  // CAUGHT message duration

// The meter fills from calm to alarm as it rises.
inline constexpr glm::vec3 kF3MeterLow  {0.420f, 0.780f, 0.878f}; // #6BC7E0
inline constexpr glm::vec3 kF3MeterHigh {0.961f, 0.345f, 0.286f}; // #F55849
inline constexpr float     kF3MeterWidth  = 260.0f;   // design pixels
inline constexpr float     kF3MeterHeight = 14.0f;

// CAMERA: a fixed angled overhead view framing the whole room at once. For a
// stealth level this beats a follow camera - the player has to see where every
// guard is to plan a route, and a camera that chases Roary hides exactly the
// half of the room they need.
inline constexpr glm::vec3 kF3CameraPos    {0.0f, 26.16f, 21.71f};
inline constexpr glm::vec3 kF3CameraTarget {0.0f, 0.0f, 2.00f};
inline constexpr float     kF3CameraFovDeg = 46.0f;

// kF3CameraFovDeg is the VERTICAL field of view at this aspect ratio. The room
// is much wider than it is deep, so on a narrower window a fixed vertical FOV
// crops the left and right walls off the screen. StealthScene widens the FOV
// below this aspect to hold the horizontal coverage steady instead - a narrow
// window then shows the same width of room with more empty space above it.
inline constexpr float kF3DesignAspect = 1.60f;  // 1280x800, the default window

// Top floor - Boss fight: crimson room, caged Pawlina, three rival mascots.
inline constexpr glm::vec3 kF4Wall    {0.353f, 0.063f, 0.188f}; // #5A1030 deep crimson
inline constexpr glm::vec3 kF4CageBar {0.604f, 0.639f, 0.678f}; // #9AA3AD silver

//=============================================================================
// TOP FLOOR GAMEPLAY - "RIVAL SHOWDOWN"
//
// A dodging fight on the open castle roof. Unlike floor 3 the arena is ROUND
// and completely empty: no walls, no cover, nowhere to hide. That is
// deliberate, and it has two nice consequences for the code -
//
//   * collision is one distance-from-the-centre check instead of a list of
//     wall slabs, so it is simpler than floor 3's, not harder
//   * with nothing to walk around, the bosses need no pathfinding at all
//
// THE PLAN (being built in pieces, like floor 3 was):
//   [x] the arena, Roary, and the dash
//   [ ] Rocky the bull: charge, miss, crash into the parapet
//   [ ] Owlsley and Sebastian joining the attack rotation
//   [ ] punish windows - touching a stunned boss is how you damage it
//
// THE DASH IS THE WHOLE FIGHT. Roary has no attack button. He moves, and he
// dashes, and that is the entire control scheme. The dash does two jobs:
//   1. it gets you out of an attack you misjudged (invincible while dashing)
//   2. it gets you IN to a stunned boss to punish it
// The long cooldown is what gives the fight its rhythm - dash in, then spend
// the recharge walking out of trouble.
//=============================================================================

inline constexpr float kF4ArenaRadius   = 11.0f;  // the walkable roof
inline constexpr float kF4DeckThickness = 0.30f;

// The low wall around the rim. Kept SHORT on purpose: a tall parapet would
// stand between the camera and the near half of the arena, which is exactly
// the problem floor 3 solved by making its near wall a lip.
inline constexpr float kF4ParapetHeight = 0.55f;
inline constexpr float kF4ParapetThick  = 0.55f;
inline constexpr int   kF4ParapetSegs   = 40;     // boxes making up the ring
inline constexpr int   kF4MerlonCount   = 20;     // battlements on top of it
inline constexpr float kF4MerlonHeight  = 0.42f;

// Deck colouring. Concentric rings rather than floor 3's checkerboard, because
// on a round arena rings tell you how far from the EDGE you are - which is the
// thing you need to know when you are backing away from a charging bull.
inline constexpr glm::vec3 kF4DeckA  {0.180f, 0.055f, 0.098f}; // #2E0E19 dark
inline constexpr glm::vec3 kF4DeckB  {0.231f, 0.075f, 0.129f}; // #3B1321 lighter
inline constexpr glm::vec3 kF4Rim    {0.482f, 0.176f, 0.278f}; // #7B2D47 rim light
inline constexpr int       kF4DeckRings = 5;

// Roary. Slightly quicker than in the stealth level - this is an action floor
// and sneaking is not a thing up here.
inline constexpr float kF4PlayerRadius = 0.42f;
inline constexpr float kF4PlayerScale  = 1.55f;
inline constexpr float kF4WalkSpeed    = 5.80f;
inline constexpr float kF4TurnSpeed    = 13.0f;
inline constexpr glm::vec3 kF4StartPos {0.0f, 0.0f, 6.0f};  // near the front rim

//-----------------------------------------------------------------------------
// THE DASH  (space bar)
//
// A short, fast burst in whichever direction he is already heading, during
// which he cannot be hurt.
//
// The numbers matter to each other, so change them together:
//   speed * duration = how far he travels (about 4.8 units here, a bit over
//   two body lengths - far enough to clear an attack, short enough that it is
//   not a teleport across the arena).
//
// The cooldown starts when the dash STARTS, not when it ends, so the full
// cycle really is kF4DashCooldown seconds end to end.
//-----------------------------------------------------------------------------
inline constexpr float kF4DashSpeed    = 22.0f;
inline constexpr float kF4DashDuration = 0.22f;
inline constexpr float kF4DashCooldown = 5.00f;


// While invincible he is washed toward this colour, which is the only signal
// the player gets that the i-frames are actually active.
inline constexpr glm::vec3 kF4InvulnTint   {0.780f, 0.925f, 1.000f}; // pale blue
inline constexpr float     kF4InvulnTintMix = 0.55f;

//-----------------------------------------------------------------------------
// HEALTH
//
// Three hits and the fight restarts. Forgiving on purpose: the fight is short,
// so the player should be learning the attack patterns rather than replaying
// the opening of it.
//-----------------------------------------------------------------------------
inline constexpr int   kF4MaxHearts    = 3;
inline constexpr float kF4HeartSize    = 20.0f;  // design pixels
inline constexpr float kF4HeartGap     = 8.0f;
inline constexpr glm::vec3 kF4HeartFull  {0.961f, 0.345f, 0.345f};
inline constexpr glm::vec3 kF4HeartEmpty {0.290f, 0.180f, 0.216f};

// Dash readiness pip, drawn next to the hearts.
inline constexpr float     kF4DashPipWidth = 120.0f;  // design pixels
inline constexpr glm::vec3 kF4DashReady    {0.420f, 0.918f, 0.784f}; // mint
inline constexpr glm::vec3 kF4DashCharging {0.400f, 0.290f, 0.333f};

//-----------------------------------------------------------------------------
// ROCKY THE BULL  -  the first boss
//
// His whole fight is one attack on a loop:
//
//   WAITING    he squares up to Roary, tracking him slowly
//   TELEGRAPH  he locks his aim and paws the ground. A lane appears on the
//              deck showing exactly where he is about to go. THIS is the
//              window the player reads - the charge itself is too fast.
//   CHARGING   straight down the locked lane, fast. Touching Roary costs a
//              heart; he does not steer, so stepping aside beats it.
//   STUNNED    he misses and hits the parapet. Wide open - touch him now and
//              he takes a hit. This is the only way to damage him.
//   RECOVER    he shakes it off and squares up again.
//
// The tuning rule behind the numbers: the telegraph has to be long enough to
// walk out of the lane WITHOUT dashing, because the dash is only available
// about one charge in two. See the note on kF4DashCooldown.
//-----------------------------------------------------------------------------
inline constexpr int   kF4RockyHealth     = 3;     // hits to beat him
inline constexpr float kF4RockyRadius     = 0.75f; // body radius, for contact
inline constexpr float kF4RockyHeight     = 2.30f; // taller than Roary
inline constexpr float kF4RockyWalkSpeed  = 1.60f; // shuffling while he waits
inline constexpr float kF4RockyTurnRate   = 2.60f; // slow - he is readable

inline constexpr float kF4RockyWaitTime      = 1.10f;
inline constexpr float kF4RockyTelegraphTime = 1.05f;  // time to step aside
inline constexpr float kF4RockyChargeSpeed   = 17.0f;
inline constexpr float kF4RockyStunTime      = 3.20f;  // the punish window
inline constexpr float kF4RockyRecoverTime   = 0.80f;

// How close Roary has to get to land a hit while Rocky is stunned. Generous:
// the player has a few seconds and a long way to run, so the last thing the
// fight needs is a fiddly contact test at the end of it.
inline constexpr float kF4RockyPunishRange = 1.70f;

// After taking a hit, Roary flashes and cannot be hit again for a moment -
// otherwise a single charge would cost two hearts as the bull slid past.
inline constexpr float kF4PlayerHitInvuln = 1.30f;

// The charge lane drawn during the telegraph. Slightly wider than Rocky so
// that standing on the edge of the drawn lane really is dangerous.
inline constexpr float     kF4LaneWidth    = 2.00f;
inline constexpr glm::vec3 kF4LaneColor    {0.961f, 0.345f, 0.286f};
inline constexpr float     kF4LaneAlpha    = 0.26f;

// Tints: white while he is reeling from a hit, red while he is stunned.
inline constexpr glm::vec3 kF4RockyHurtTint {1.000f, 1.000f, 1.000f};
inline constexpr glm::vec3 kF4RockyStunTint {0.961f, 0.549f, 0.286f};

inline constexpr glm::vec3 kF4BossBarFill  {0.839f, 0.247f, 0.306f};
inline constexpr glm::vec3 kF4BossBarTrack {0.290f, 0.129f, 0.173f};
inline constexpr float     kF4BossBarWidth = 300.0f;  // design pixels

// How long the win and lose messages stay up before the level reacts.
inline constexpr float kF4VictoryTime = 2.60f;
inline constexpr float kF4DefeatTime  = 2.20f;

//-----------------------------------------------------------------------------
// CAMERA  (solved the same way floor 3's was - see kF3DesignAspect)
//-----------------------------------------------------------------------------
// Solved by projecting the arena rim through the real Camera and searching
// for the framing that fills the most screen while staying clear of the title
// and the hint lines. 55 degrees rather than the slightly roomier 43: a round
// arena seen from a shallow angle squashes into a flat ellipse, and judging
// how far away a charging bull is gets much harder. The steeper view costs
// about 7% of the screen width and is worth it.
inline constexpr glm::vec3 kF4CameraPos    {0.0f, 27.44f, 21.71f};
inline constexpr glm::vec3 kF4CameraTarget {0.0f, 0.0f, 2.50f};
inline constexpr float     kF4CameraFovDeg = 46.0f;


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
// FLOOR 1 - PLATFORMER TUNING
//
// Jump feel is driven by two numbers a designer can actually picture: how high
// the jump goes, and how long it takes to get there. Gravity and launch speed
// are DERIVED from those, so you never tune a gravity constant by trial and
// error - change kJumpHeight and the jump still reaches exactly that height.
//-----------------------------------------------------------------------------
namespace plat {

inline constexpr float kJumpHeight = 2.30f;  // world units cleared by a full jump
inline constexpr float kTimeToApex = 0.36f;  // seconds to reach the top

// Standard projectile motion solved for gravity and launch speed.
inline constexpr float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline constexpr float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }

// Falling under heavier gravity than rising is the single cheapest way to stop
// a jump feeling floaty.
inline constexpr float kFallGravityMultiplier = 1.85f;

// Releasing the jump button while still rising cuts the climb short, which is
// what gives you both a tap-hop and a full jump from one button.
inline constexpr float kJumpCutMultiplier = 0.45f;

// Two forgiveness windows. Coyote time lets you jump just after walking off an
// edge; the buffer lets a press land slightly before you touch down and still
// fire. Without them the controls feel broken even when they are correct.
inline constexpr float kCoyoteTime     = 0.12f;
inline constexpr float kJumpBufferTime = 0.12f;

// Horizontal movement. Separate rates for speeding up, slowing down and
// turning around, and weaker versions of each in the air.
inline constexpr float kMaxRunSpeed  = 6.4f;
inline constexpr float kGroundAccel  = 60.0f;
inline constexpr float kGroundDecel  = 55.0f;
inline constexpr float kGroundTurn   = 95.0f;
inline constexpr float kAirAccel     = 34.0f;
inline constexpr float kAirDecel     = 12.0f;
inline constexpr float kAirTurn      = 50.0f;

// Roary's collision box. Slightly narrower than he looks, which makes landing
// on the edge of a platform feel generous rather than fussy.
inline constexpr glm::vec3 kPlayerSize {0.70f, 1.30f, 0.70f};

// Camera: a fixed angle that follows, rather than one the player rotates.
inline constexpr glm::vec3 kCameraOffset {0.0f, 6.2f, 15.5f};
inline constexpr glm::vec3 kCameraLookAt {0.0f, 1.1f, 0.0f};
inline constexpr float     kCameraFovDeg = 52.0f;
inline constexpr float     kCameraFollow = 6.5f;   // higher = tighter follow

// Level colors.
inline constexpr glm::vec3 kPlatformWarm = kF1BlockWarm;
inline constexpr glm::vec3 kPlatformCool = kF1BlockCool;
inline constexpr glm::vec3 kPlatformEdge {0.180f, 0.220f, 0.290f}; // side faces
inline constexpr glm::vec3 kMovingPlatform {0.976f, 0.729f, 0.247f}; // #F9BA3F
inline constexpr glm::vec3 kGoalColor      {0.345f, 0.957f, 0.667f}; // #58F4AA
inline constexpr glm::vec3 kShadowColor    {0.020f, 0.040f, 0.080f};

// --- enemies and stomping ---------------------------------------------------
// The stomp is the mechanic the whole genre is built on: land on something and
// it dies, and you bounce. Getting the bounce right matters - too little and
// it feels like a stumble, too much and you lose control.
inline constexpr float kStompBounceSpeed = 10.5f;

// How far into the enemy's top you must be for it to count as a stomp rather
// than a side hit. Generous on purpose: a near miss reading as a death is the
// most infuriating thing a platformer can do.
inline constexpr float kStompGrace = 0.40f;

inline constexpr int   kStartingLives    = 3;
inline constexpr float kInvulnerableTime = 1.60f;  // after taking a hit
inline constexpr float kHitKnockback     = 7.0f;
inline constexpr float kHitKnockUp       = 7.5f;
inline constexpr float kEnemySquashTime  = 0.32f;  // how long the squash shows

inline constexpr glm::vec3 kEnemySize      {0.78f, 0.72f, 0.78f};
inline constexpr glm::vec3 kWalkerBody     {0.682f, 0.286f, 0.447f}; // #AE4972
inline constexpr glm::vec3 kWalkerFoot     {0.400f, 0.145f, 0.255f}; // #662541
inline constexpr glm::vec3 kSpikerBody     {0.306f, 0.275f, 0.420f}; // #4E466B
inline constexpr glm::vec3 kSpikerSpike    {0.878f, 0.906f, 0.937f}; // pale spikes

// --- item blocks ------------------------------------------------------------
// Solid from every side, but punching the underside pops a coin out. The used
// block dims so you can see at a glance which are spent.
inline constexpr glm::vec3 kItemBlockSize  {1.20f, 1.20f, 1.20f};
inline constexpr glm::vec3 kItemBlockLive  {0.961f, 0.690f, 0.196f}; // #F5B032
inline constexpr glm::vec3 kItemBlockTrim  {0.996f, 0.886f, 0.553f};
inline constexpr glm::vec3 kItemBlockSpent {0.325f, 0.306f, 0.282f}; // #534E48
inline constexpr float     kPoppedCoinTime = 0.70f;
inline constexpr float     kPoppedCoinRise = 1.90f;

// --- bounce pads ------------------------------------------------------------
inline constexpr float     kBouncePadSpeed = 17.0f;  // reaches ~4.1 units up
inline constexpr glm::vec3 kBouncePadColor {0.400f, 0.878f, 0.945f}; // #66E0F1
inline constexpr float     kBouncePadSquash = 0.22f; // animation length

inline constexpr float kCoinRadius   = 0.40f;  // how close counts as collected
inline constexpr float kCoinSpinRate = 110.0f; // degrees per second
inline constexpr float kGoalHoldTime = 1.40f;  // celebration before returning

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

inline constexpr float kTunnelSide    = 6.0f;   // the square cross-section
inline constexpr int   kLanesPerFace  = 3;
inline constexpr float kSegmentLength = 2.4f;   // depth of one row of tiles
inline constexpr float kTileThickness = 0.45f;

inline constexpr float laneWidth() { return kTunnelSide / static_cast<float>(kLanesPerFace); }
inline constexpr float halfSide()  { return kTunnelSide * 0.5f; }

// Auto-run. The speed creeps up so the level gets harder without new content.
inline constexpr float kStartSpeed = 11.0f;
inline constexpr float kMaxSpeed   = 16.5f;
inline constexpr float kSpeedRamp  = 0.22f;   // units/sec gained per second

// Strafing across the face you are on.
inline constexpr float kStrafeSpeed = 9.0f;
inline constexpr float kStrafeAccel = 70.0f;

// Jump, derived the same way as the platformer: pick the height and the time
// to reach it, and gravity follows.
inline constexpr float kJumpHeight = 2.20f;
inline constexpr float kTimeToApex = 0.30f;
inline constexpr float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline constexpr float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }
inline constexpr float kFallGravityMultiplier = 1.70f;
inline constexpr float kCoyoteTime = 0.10f;

// Falling this far past the surface means you missed, and the run restarts.
inline constexpr float kFallLimit = 7.0f;

// How fast the view rolls when you change face. Quick enough to keep up, slow
// enough that you can see which way the world turned.
inline constexpr float kRollSpeed = 420.0f;     // degrees per second

// Camera sits behind and above, measured in the CURRENT face's frame so it
// rolls with you.
inline constexpr float kCameraBack   = 7.5f;
inline constexpr float kCameraUp     = 2.6f;
inline constexpr float kCameraAhead  = 6.0f;
inline constexpr float kCameraFovDeg = 62.0f;

// Colors. The tunnel is the dark floor of the tower, lit by its own edges.
inline constexpr glm::vec3 kTileTopA   {0.541f, 0.333f, 0.776f}; // #8A55C6 violet
inline constexpr glm::vec3 kTileTopB   {0.345f, 0.776f, 0.871f}; // #58C6DE cyan
inline constexpr glm::vec3 kTileBody   {0.094f, 0.063f, 0.157f}; // #180F28
inline constexpr glm::vec3 kVoidColor  {0.020f, 0.012f, 0.039f};
inline constexpr glm::vec3 kGoalRing   {0.400f, 0.976f, 0.706f};

// How many segments ahead are drawn. Beyond this the tunnel fades into dark.
inline constexpr int kDrawDistance = 26;

inline constexpr glm::vec3 kPlayerSize {0.80f, 1.30f, 0.80f};

} // namespace tunnel

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
