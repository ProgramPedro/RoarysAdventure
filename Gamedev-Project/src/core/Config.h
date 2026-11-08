#pragma once

// Every color, size, position, speed, tuning value and asset path the game uses,
// gathered into one namespace so nothing else in the project hardcodes a
// measurement. Colors are linear RGB in the 0..1 range. This is the file to edit
// to retheme the game or change how a level feels.

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace cfg {
const int  kWindowWidth   = 1280;
const int  kWindowHeight  = 800;
const char kWindowTitle[] = "Roary's Adventure: Save Pawlina!";

const glm::vec3 kBgTop     {0.031f, 0.118f, 0.247f};
const glm::vec3 kBgBottom  {0.008f, 0.035f, 0.086f};
const glm::vec3 kBgGlow    {0.714f, 0.525f, 0.173f};
const glm::vec2 kBgGlowPos {0.0f, 0.35f};
const float     kBgGlowRadius   = 1.15f;
const float     kBgGlowStrength = 0.55f;

const glm::vec3 kFiuGold {0.839f, 0.682f, 0.216f};
const glm::vec3 kFiuBlue {0.090f, 0.251f, 0.494f};

const glm::vec3 kTextPrimary   {0.960f, 0.960f, 0.980f};
const glm::vec3 kTextDim       {0.600f, 0.640f, 0.720f};
const glm::vec3 kTextLocked    {0.820f, 0.380f, 0.380f};
const glm::vec3 kTextComplete  {0.380f, 0.880f, 0.560f};

const float kTitleScale       = 7.0f;
const float kSubtitleScale    = 4.0f;
const float kFloorNameScale   = 4.0f;
const float kDescriptionScale = 2.0f;
const float kHintScale        = 2.0f;
const float kFloorLabelScale  = 2.0f;

const glm::vec3 kTowerStone     {0.290f, 0.306f, 0.341f};
const glm::vec3 kTowerStoneDark {0.192f, 0.204f, 0.235f};
const glm::vec3 kTowerBase      {0.235f, 0.212f, 0.192f};
const glm::vec3 kTowerMerlon    {0.482f, 0.506f, 0.553f};

const glm::vec3 kLockedGrey = {0.180f, 0.188f, 0.204f};
const float     kLockedFade = 0.60f;

const glm::vec3 kSelectFrame      {1.000f, 0.960f, 0.870f};
const float     kSelectPulseSpeed = 3.2f;
const float     kSelectFrameThick = 0.12f;

const int   kFloorCount   = 4;
const float kFloorHeight  = 2.30f;
const float kFloorGap     = 0.12f;
const float kTowerBaseY   = 0.0f;

inline constexpr float kFloorWidth  = 7.0f;
inline constexpr float kTowerRadius = kFloorWidth * 0.5f;

inline float floorCenterY(int floorIndex) {
    return kTowerBaseY + kFloorHeight * 0.5f
         + static_cast<float>(floorIndex) * (kFloorHeight + kFloorGap);
}

const float kSlabThickness = 0.22f;
const float kPillarWidth   = 0.42f;
const float kWallThickness = 0.45f;
const float kPropZ         = 0.0f;

const int   kWallSegments  = 18;

const float kWallOpeningDegrees = 112.0f;

const float kSlabOverhang = 0.28f;

const float kParapetHeight = 0.55f;
const float kMerlonHeight  = 0.62f;
const float kMerlonWidth   = 0.78f;
const int   kMerlonCount   = 10;

const float kBossDaisHeight = 0.40f;
const float kBossDaisRadius = 3.00f;

const float kTopFloorPickHeight = 1.90f;

const float kUiReferenceHeight = 800.0f;

const glm::vec3 kMenuCameraPos    {5.8f, 4.9f, 25.8f};
const glm::vec3 kMenuCameraTarget {-0.2f, 2.62f, 0.0f};
const float     kMenuCameraFovDeg = 42.0f;
const float     kCameraNear       = 0.1f;
const float     kCameraFar        = 100.0f;

const float kCameraSwayAmount = 0.30f;
const float kCameraSwaySpeed  = 0.35f;

const glm::vec3 kLightDirection {-0.45f, -0.80f, -0.40f};

const glm::vec3 kPantherFur   {0.784f, 0.576f, 0.333f};
const glm::vec3 kPantherCream {0.910f, 0.824f, 0.682f};
const glm::vec3 kPantherInner {0.549f, 0.369f, 0.200f};
const glm::vec3 kPantherNose  {0.227f, 0.165f, 0.133f};
const glm::vec3 kEyeDark      {0.102f, 0.094f, 0.110f};
const glm::vec3 kEyeWhite     {0.965f, 0.961f, 0.953f};

const glm::vec3 kFiuKit       {0.086f, 0.145f, 0.353f};
const glm::vec3 kFiuKitTrim   {0.950f, 0.950f, 0.960f};
const glm::vec3 kPawlinaTrim  {0.788f, 0.635f, 0.153f};

const glm::vec3 kOwlBody   {0.290f, 0.204f, 0.149f};
const glm::vec3 kOwlCream  {0.847f, 0.773f, 0.659f};
const glm::vec3 kOwlBrow   {0.949f, 0.361f, 0.098f};
const glm::vec3 kOwlEye    {0.180f, 0.545f, 0.831f};
const glm::vec3 kOwlJersey {0.169f, 0.333f, 0.722f};
const glm::vec3 kOwlPants  {0.925f, 0.925f, 0.937f};

const glm::vec3 kIbisBody   {0.949f, 0.941f, 0.918f};
const glm::vec3 kIbisBill   {0.910f, 0.384f, 0.165f};
const glm::vec3 kIbisJersey {0.957f, 0.451f, 0.129f};
const glm::vec3 kIbisFeet   {0.949f, 0.761f, 0.188f};
const glm::vec3 kIbisGreen  {0.000f, 0.314f, 0.188f};

const glm::vec3 kBullBody   {0.608f, 0.608f, 0.588f};
const glm::vec3 kBullMuzzle {0.749f, 0.741f, 0.714f};
const glm::vec3 kBullHorn   {0.831f, 0.686f, 0.216f};
const glm::vec3 kBullJersey {0.043f, 0.478f, 0.310f};
const glm::vec3 kBullPants  {0.812f, 0.769f, 0.576f};
const glm::vec3 kBullHoof   {0.129f, 0.125f, 0.133f};

const glm::vec3 kRoaryAccent = kFiuGold;
const glm::vec3 kRoarySize   {1.00f, 1.42f, 1.00f};

const float kRoaryOffsetX   = -kTowerRadius - 1.25f;
const float kRoaryOffsetZ   = 1.1f;
const float kRoaryMoveSpeed = 8.0f;
const float kRoaryBobAmount = 0.09f;
const float kRoaryBobSpeed  = 2.6f;

const glm::vec3 kF1Wall      {0.333f, 0.447f, 0.600f};
const glm::vec3 kF1BlockWarm {1.000f, 0.478f, 0.302f};
const glm::vec3 kF1BlockCool {0.310f, 0.878f, 0.627f};
const glm::vec3 kF1Coin      {1.000f, 0.835f, 0.310f};

const glm::vec3 kF2Wall    {0.078f, 0.047f, 0.118f};
const glm::vec3 kF2Tunnel  {0.027f, 0.016f, 0.047f};
const glm::vec3 kF2RimGlow {0.420f, 0.247f, 0.627f};
const glm::vec3 kF2Accent  {0.541f, 0.886f, 0.937f};

const glm::vec3 kF3Wall        {0.082f, 0.200f, 0.227f};
const glm::vec3 kF3Light       {0.812f, 0.965f, 1.000f};
const glm::vec3 kF3Guard       {0.047f, 0.059f, 0.071f};
const float     kF3LightAlpha  = 0.34f;
const float     kF3SweepSpeed  = 0.9f;
const float     kF3SweepDeg    = 26.0f;

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
const float kF3SneakVisionMul     = 0.6f;

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
const float kF3RockThrowRange = 8.0f;

const float kF3NoiseRadius   = 5.50f;

const float kF3GuardAlertTime = 4.00f;

const glm::vec3 kF3ConeAlerted {0.949f, 0.639f, 0.212f};

struct F3PatrolPoint {
    glm::vec3 position;
    float     pauseSeconds;
};

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

const float     kF3CaughtFlashTime = 1.00f;
const glm::vec3 kF3CaughtTint {0.961f, 0.345f, 0.286f};

const glm::vec3 kF3CameraPos    {0.0f, 26.16f, 21.71f};
const glm::vec3 kF3CameraTarget {0.0f, 0.0f, 2.00f};
const float     kF3CameraFovDeg = 46.0f;

const glm::vec3 kF4Wall    {0.353f, 0.063f, 0.188f};
const glm::vec3 kF4CageBar {0.604f, 0.639f, 0.678f};

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

struct FloorInfo {
    const char* name;
    const char* description;
    const char* shortLabel;
};

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

namespace plat {
const float kJumpHeight = 2.30f;
const float kTimeToApex = 0.36f;

inline float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }

const float kFallGravityMultiplier = 1.85f;

const float kJumpCutMultiplier = 0.45f;

const float kCoyoteTime     = 0.12f;
const float kJumpBufferTime = 0.12f;

const float kMaxRunSpeed  = 6.4f;
const float kGroundAccel  = 60.0f;
const float kGroundDecel  = 55.0f;
const float kGroundTurn   = 95.0f;
const float kAirAccel     = 34.0f;
const float kAirDecel     = 12.0f;
const float kAirTurn      = 50.0f;

const glm::vec3 kPlayerSize {0.70f, 1.30f, 0.70f};

const glm::vec3 kCameraOffset {0.0f, 6.2f, 15.5f};
const glm::vec3 kCameraLookAt {0.0f, 1.1f, 0.0f};
const float     kCameraFovDeg = 52.0f;
const float     kCameraFollow = 6.5f;

const glm::vec3 kPlatformWarm = kF1BlockWarm;
const glm::vec3 kPlatformCool = kF1BlockCool;
const glm::vec3 kPlatformEdge {0.180f, 0.220f, 0.290f};
const glm::vec3 kMovingPlatform {0.976f, 0.729f, 0.247f};
const glm::vec3 kGoalColor      {0.345f, 0.957f, 0.667f};
const glm::vec3 kShadowColor    {0.020f, 0.040f, 0.080f};

const float kStompBounceSpeed = 10.5f;

const float kStompGrace = 0.40f;

const int   kStartingLives    = 3;
const float kInvulnerableTime = 1.60f;
const float kHitKnockback     = 7.0f;
const float kHitKnockUp       = 7.5f;
const float kEnemySquashTime  = 0.32f;

const glm::vec3 kEnemySize      {0.78f, 0.72f, 0.78f};
const glm::vec3 kWalkerBody     {0.682f, 0.286f, 0.447f};
const glm::vec3 kWalkerFoot     {0.400f, 0.145f, 0.255f};
const glm::vec3 kSpikerBody     {0.306f, 0.275f, 0.420f};
const glm::vec3 kSpikerSpike    {0.878f, 0.906f, 0.937f};

const glm::vec3 kItemBlockSize  {1.20f, 1.20f, 1.20f};
const glm::vec3 kItemBlockLive  {0.961f, 0.690f, 0.196f};
const glm::vec3 kItemBlockTrim  {0.996f, 0.886f, 0.553f};
const glm::vec3 kItemBlockSpent {0.325f, 0.306f, 0.282f};
const float     kPoppedCoinTime = 0.70f;
const float     kPoppedCoinRise = 1.90f;

const float     kBouncePadSpeed = 17.0f;
const glm::vec3 kBouncePadColor {0.400f, 0.878f, 0.945f};
const float     kBouncePadSquash = 0.22f;

const float kCoinRadius   = 0.40f;
const float kCoinSpinRate = 110.0f;
const float kGoalHoldTime = 1.40f;

}

namespace tunnel {
const float kTunnelSide    = 6.0f;
const int   kLanesPerFace  = 3;
const float kSegmentLength = 2.4f;
const float kTileThickness = 0.45f;

inline float laneWidth() { return kTunnelSide / static_cast<float>(kLanesPerFace); }
inline float halfSide()  { return kTunnelSide * 0.5f; }

const float kStartSpeed = 11.0f;
const float kMaxSpeed   = 16.5f;
const float kSpeedRamp  = 0.22f;

const float kStrafeSpeed = 9.0f;
const float kStrafeAccel = 70.0f;

const float kJumpHeight = 2.20f;
const float kTimeToApex = 0.30f;
inline float gravity()   { return 2.0f * kJumpHeight / (kTimeToApex * kTimeToApex); }
inline float jumpSpeed() { return 2.0f * kJumpHeight / kTimeToApex; }
const float kFallGravityMultiplier = 1.70f;
const float kCoyoteTime = 0.10f;

const float kFallLimit = 7.0f;

const float kRollSpeed = 420.0f;

const float kCameraBack   = 7.5f;
const float kCameraUp     = 2.6f;
const float kCameraAhead  = 6.0f;
const float kCameraFovDeg = 62.0f;

const glm::vec3 kTileTopA   {0.541f, 0.333f, 0.776f};
const glm::vec3 kTileTopB   {0.345f, 0.776f, 0.871f};
const glm::vec3 kTileBody   {0.094f, 0.063f, 0.157f};
const glm::vec3 kVoidColor  {0.020f, 0.012f, 0.039f};
const glm::vec3 kGoalRing   {0.400f, 0.976f, 0.706f};

const int kDrawDistance = 26;

const glm::vec3 kPlayerSize {0.80f, 1.30f, 0.80f};

}

}
