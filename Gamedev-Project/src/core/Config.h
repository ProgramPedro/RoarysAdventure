#pragma once

// Every color, size, position, speed, tuning value and asset path the game uses,
// gathered into one namespace so nothing else in the project hardcodes a
// measurement. Colors are linear RGB in the 0..1 range. This is the file to edit
// to retheme the game or change how a level feels.

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace cfg {
inline constexpr int  kWindowWidth   = 1280;
inline constexpr int  kWindowHeight  = 800;
inline constexpr char kWindowTitle[] = "Roary's Adventure: Save Pawlina!";

inline constexpr glm::vec3 kBgTop     {0.031f, 0.118f, 0.247f};
inline constexpr glm::vec3 kBgBottom  {0.008f, 0.035f, 0.086f};
inline constexpr glm::vec3 kBgGlow    {0.714f, 0.525f, 0.173f};
inline constexpr glm::vec2 kBgGlowPos {0.0f, 0.35f};
inline constexpr float     kBgGlowRadius   = 1.15f;
inline constexpr float     kBgGlowStrength = 0.55f;

inline constexpr glm::vec3 kFiuGold {0.839f, 0.682f, 0.216f};
inline constexpr glm::vec3 kFiuBlue {0.090f, 0.251f, 0.494f};

inline constexpr glm::vec3 kTextPrimary   {0.960f, 0.960f, 0.980f};
inline constexpr glm::vec3 kTextDim       {0.600f, 0.640f, 0.720f};
inline constexpr glm::vec3 kTextLocked    {0.820f, 0.380f, 0.380f};
inline constexpr glm::vec3 kTextComplete  {0.380f, 0.880f, 0.560f};

inline constexpr float kTitleScale       = 7.0f;
inline constexpr float kSubtitleScale    = 4.0f;
inline constexpr float kFloorNameScale   = 4.0f;
inline constexpr float kDescriptionScale = 2.0f;
inline constexpr float kHintScale        = 2.0f;
inline constexpr float kFloorLabelScale  = 2.0f;

inline constexpr glm::vec3 kTowerStone     {0.290f, 0.306f, 0.341f};
inline constexpr glm::vec3 kTowerStoneDark {0.192f, 0.204f, 0.235f};
inline constexpr glm::vec3 kTowerBase      {0.235f, 0.212f, 0.192f};
inline constexpr glm::vec3 kTowerMerlon    {0.482f, 0.506f, 0.553f};

inline constexpr glm::vec3 kLockedGrey = {0.180f, 0.188f, 0.204f};
inline constexpr float     kLockedFade = 0.60f;

inline constexpr glm::vec3 kSelectFrame      {1.000f, 0.960f, 0.870f};
inline constexpr float     kSelectPulseSpeed = 3.2f;
inline constexpr float     kSelectFrameThick = 0.12f;

inline constexpr int   kFloorCount   = 4;
inline constexpr float kFloorWidth   = 7.0f;
inline constexpr float kFloorHeight  = 2.30f;
inline constexpr float kFloorGap     = 0.12f;
inline constexpr float kTowerBaseY   = 0.0f;

inline constexpr float kTowerRadius  = kFloorWidth * 0.5f;

inline constexpr float floorCenterY(int floorIndex) {
    return kTowerBaseY + kFloorHeight * 0.5f
         + static_cast<float>(floorIndex) * (kFloorHeight + kFloorGap);
}

inline constexpr float kSlabThickness = 0.22f;
inline constexpr float kPillarWidth   = 0.42f;
inline constexpr float kWallThickness = 0.45f;
inline constexpr float kPropZ         = 0.0f;

inline constexpr int   kWallSegments  = 18;

inline constexpr float kWallOpeningDegrees = 112.0f;

inline constexpr float kSlabOverhang = 0.28f;

inline constexpr float kParapetHeight = 0.55f;
inline constexpr float kMerlonHeight  = 0.62f;
inline constexpr float kMerlonWidth   = 0.78f;
inline constexpr int   kMerlonCount   = 10;

inline constexpr float kBossDaisHeight = 0.40f;
inline constexpr float kBossDaisRadius = 3.00f;

inline constexpr float kTopFloorPickHeight = 1.90f;

inline constexpr float kUiReferenceHeight = 800.0f;

inline constexpr glm::vec3 kMenuCameraPos    {5.8f, 4.9f, 25.8f};
inline constexpr glm::vec3 kMenuCameraTarget {-0.2f, 2.62f, 0.0f};
inline constexpr float     kMenuCameraFovDeg = 42.0f;
inline constexpr float     kCameraNear       = 0.1f;
inline constexpr float     kCameraFar        = 100.0f;

inline constexpr float kCameraSwayAmount = 0.30f;
inline constexpr float kCameraSwaySpeed  = 0.35f;

inline constexpr glm::vec3 kLightDirection {-0.45f, -0.80f, -0.40f};

inline constexpr glm::vec3 kPantherFur   {0.784f, 0.576f, 0.333f};
inline constexpr glm::vec3 kPantherCream {0.910f, 0.824f, 0.682f};
inline constexpr glm::vec3 kPantherInner {0.549f, 0.369f, 0.200f};
inline constexpr glm::vec3 kPantherNose  {0.227f, 0.165f, 0.133f};
inline constexpr glm::vec3 kEyeDark      {0.102f, 0.094f, 0.110f};
inline constexpr glm::vec3 kEyeWhite     {0.965f, 0.961f, 0.953f};

inline constexpr glm::vec3 kFiuKit       {0.086f, 0.145f, 0.353f};
inline constexpr glm::vec3 kFiuKitTrim   {0.950f, 0.950f, 0.960f};
inline constexpr glm::vec3 kPawlinaTrim  {0.788f, 0.635f, 0.153f};

inline constexpr glm::vec3 kOwlBody   {0.290f, 0.204f, 0.149f};
inline constexpr glm::vec3 kOwlCream  {0.847f, 0.773f, 0.659f};
inline constexpr glm::vec3 kOwlBrow   {0.949f, 0.361f, 0.098f};
inline constexpr glm::vec3 kOwlEye    {0.180f, 0.545f, 0.831f};
inline constexpr glm::vec3 kOwlJersey {0.169f, 0.333f, 0.722f};
inline constexpr glm::vec3 kOwlPants  {0.925f, 0.925f, 0.937f};

inline constexpr glm::vec3 kIbisBody   {0.949f, 0.941f, 0.918f};
inline constexpr glm::vec3 kIbisBill   {0.910f, 0.384f, 0.165f};
inline constexpr glm::vec3 kIbisJersey {0.957f, 0.451f, 0.129f};
inline constexpr glm::vec3 kIbisFeet   {0.949f, 0.761f, 0.188f};
inline constexpr glm::vec3 kIbisGreen  {0.000f, 0.314f, 0.188f};

inline constexpr glm::vec3 kBullBody   {0.608f, 0.608f, 0.588f};
inline constexpr glm::vec3 kBullMuzzle {0.749f, 0.741f, 0.714f};
inline constexpr glm::vec3 kBullHorn   {0.831f, 0.686f, 0.216f};
inline constexpr glm::vec3 kBullJersey {0.043f, 0.478f, 0.310f};
inline constexpr glm::vec3 kBullPants  {0.812f, 0.769f, 0.576f};
inline constexpr glm::vec3 kBullHoof   {0.129f, 0.125f, 0.133f};

inline constexpr glm::vec3 kRoaryAccent = kFiuGold;
inline constexpr glm::vec3 kRoarySize   {1.00f, 1.42f, 1.00f};

inline constexpr float kRoaryOffsetX   = -kTowerRadius - 1.25f;
inline constexpr float kRoaryOffsetZ   = 1.1f;
inline constexpr float kRoaryMoveSpeed = 8.0f;
inline constexpr float kRoaryBobAmount = 0.09f;
inline constexpr float kRoaryBobSpeed  = 2.6f;

inline constexpr glm::vec3 kF1Wall      {0.333f, 0.447f, 0.600f};
inline constexpr glm::vec3 kF1BlockWarm {1.000f, 0.478f, 0.302f};
inline constexpr glm::vec3 kF1BlockCool {0.310f, 0.878f, 0.627f};
inline constexpr glm::vec3 kF1Coin      {1.000f, 0.835f, 0.310f};

inline constexpr glm::vec3 kF2Wall    {0.078f, 0.047f, 0.118f};
inline constexpr glm::vec3 kF2Tunnel  {0.027f, 0.016f, 0.047f};
inline constexpr glm::vec3 kF2RimGlow {0.420f, 0.247f, 0.627f};
inline constexpr glm::vec3 kF2Accent  {0.541f, 0.886f, 0.937f};

inline constexpr glm::vec3 kF3Wall        {0.082f, 0.200f, 0.227f};
inline constexpr glm::vec3 kF3Light       {0.812f, 0.965f, 1.000f};
inline constexpr glm::vec3 kF3Guard       {0.047f, 0.059f, 0.071f};
inline constexpr float     kF3LightAlpha  = 0.34f;
inline constexpr float     kF3SweepSpeed  = 0.9f;
inline constexpr float     kF3SweepDeg    = 26.0f;

inline constexpr glm::vec3 kF4Wall    {0.353f, 0.063f, 0.188f};
inline constexpr glm::vec3 kF4CageBar {0.604f, 0.639f, 0.678f};

struct FloorInfo {
    const char* name;
    const char* description;
    const char* shortLabel;
};

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

namespace plat {
inline constexpr float kJumpHeight = 2.30f;
inline constexpr float kTimeToApex = 0.36f;

inline constexpr float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline constexpr float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }

inline constexpr float kFallGravityMultiplier = 1.85f;

inline constexpr float kJumpCutMultiplier = 0.45f;

inline constexpr float kCoyoteTime     = 0.12f;
inline constexpr float kJumpBufferTime = 0.12f;

inline constexpr float kMaxRunSpeed  = 6.4f;
inline constexpr float kGroundAccel  = 60.0f;
inline constexpr float kGroundDecel  = 55.0f;
inline constexpr float kGroundTurn   = 95.0f;
inline constexpr float kAirAccel     = 34.0f;
inline constexpr float kAirDecel     = 12.0f;
inline constexpr float kAirTurn      = 50.0f;

inline constexpr glm::vec3 kPlayerSize {0.70f, 1.30f, 0.70f};

inline constexpr glm::vec3 kCameraOffset {0.0f, 6.2f, 15.5f};
inline constexpr glm::vec3 kCameraLookAt {0.0f, 1.1f, 0.0f};
inline constexpr float     kCameraFovDeg = 52.0f;
inline constexpr float     kCameraFollow = 6.5f;

inline constexpr glm::vec3 kPlatformWarm = kF1BlockWarm;
inline constexpr glm::vec3 kPlatformCool = kF1BlockCool;
inline constexpr glm::vec3 kPlatformEdge {0.180f, 0.220f, 0.290f};
inline constexpr glm::vec3 kMovingPlatform {0.976f, 0.729f, 0.247f};
inline constexpr glm::vec3 kGoalColor      {0.345f, 0.957f, 0.667f};
inline constexpr glm::vec3 kShadowColor    {0.020f, 0.040f, 0.080f};

inline constexpr float kStompBounceSpeed = 10.5f;

inline constexpr float kStompGrace = 0.40f;

inline constexpr int   kStartingLives    = 3;
inline constexpr float kInvulnerableTime = 1.60f;
inline constexpr float kHitKnockback     = 7.0f;
inline constexpr float kHitKnockUp       = 7.5f;
inline constexpr float kEnemySquashTime  = 0.32f;

inline constexpr glm::vec3 kEnemySize      {0.78f, 0.72f, 0.78f};
inline constexpr glm::vec3 kWalkerBody     {0.682f, 0.286f, 0.447f};
inline constexpr glm::vec3 kWalkerFoot     {0.400f, 0.145f, 0.255f};
inline constexpr glm::vec3 kSpikerBody     {0.306f, 0.275f, 0.420f};
inline constexpr glm::vec3 kSpikerSpike    {0.878f, 0.906f, 0.937f};

inline constexpr glm::vec3 kItemBlockSize  {1.20f, 1.20f, 1.20f};
inline constexpr glm::vec3 kItemBlockLive  {0.961f, 0.690f, 0.196f};
inline constexpr glm::vec3 kItemBlockTrim  {0.996f, 0.886f, 0.553f};
inline constexpr glm::vec3 kItemBlockSpent {0.325f, 0.306f, 0.282f};
inline constexpr float     kPoppedCoinTime = 0.70f;
inline constexpr float     kPoppedCoinRise = 1.90f;

inline constexpr float     kBouncePadSpeed = 17.0f;
inline constexpr glm::vec3 kBouncePadColor {0.400f, 0.878f, 0.945f};
inline constexpr float     kBouncePadSquash = 0.22f;

inline constexpr float kCoinRadius   = 0.40f;
inline constexpr float kCoinSpinRate = 110.0f;
inline constexpr float kGoalHoldTime = 1.40f;

}

namespace tunnel {
inline constexpr float kTunnelSide    = 6.0f;
inline constexpr int   kLanesPerFace  = 3;
inline constexpr float kSegmentLength = 2.4f;
inline constexpr float kTileThickness = 0.45f;

inline constexpr float laneWidth() { return kTunnelSide / static_cast<float>(kLanesPerFace); }
inline constexpr float halfSide()  { return kTunnelSide * 0.5f; }

inline constexpr float kStartSpeed = 11.0f;
inline constexpr float kMaxSpeed   = 16.5f;
inline constexpr float kSpeedRamp  = 0.22f;

inline constexpr float kStrafeSpeed = 9.0f;
inline constexpr float kStrafeAccel = 70.0f;

inline constexpr float kJumpHeight = 2.20f;
inline constexpr float kTimeToApex = 0.30f;
inline constexpr float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline constexpr float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }
inline constexpr float kFallGravityMultiplier = 1.70f;
inline constexpr float kCoyoteTime = 0.10f;

inline constexpr float kFallLimit = 7.0f;

inline constexpr float kRollSpeed = 420.0f;

inline constexpr float kCameraBack   = 7.5f;
inline constexpr float kCameraUp     = 2.6f;
inline constexpr float kCameraAhead  = 6.0f;
inline constexpr float kCameraFovDeg = 62.0f;

inline constexpr glm::vec3 kTileTopA   {0.541f, 0.333f, 0.776f};
inline constexpr glm::vec3 kTileTopB   {0.345f, 0.776f, 0.871f};
inline constexpr glm::vec3 kTileBody   {0.094f, 0.063f, 0.157f};
inline constexpr glm::vec3 kVoidColor  {0.020f, 0.012f, 0.039f};
inline constexpr glm::vec3 kGoalRing   {0.400f, 0.976f, 0.706f};

inline constexpr int kDrawDistance = 26;

inline constexpr glm::vec3 kPlayerSize {0.80f, 1.30f, 0.80f};

}

inline constexpr char kAssetRoary[]   = "assets/roary.png";
inline constexpr char kAssetPawlina[] = "assets/pawlina.png";
inline constexpr char kAssetStone[]   = "assets/stone.png";

}
