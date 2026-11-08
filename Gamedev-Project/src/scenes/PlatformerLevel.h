#pragma once

// The layout of Floor 1, written as plain tables: static platforms, moving
// platforms, enemies, coins, item blocks and bounce pads. Adding a row here puts
// the thing in the game, with no code to change. The spacing is chosen against
// the jump and bounce distances in cfg::plat so every piece stays reachable.

#include <glm/vec3.hpp>

namespace level1 {
struct PlatformDef {
    glm::vec3 center;
    glm::vec3 size;
    bool warm;
};

inline constexpr PlatformDef kPlatforms[] = {
    {{ -3.0f, -0.50f,  0.0f}, {12.0f, 1.00f, 7.0f}, false},
    {{  9.0f, -0.50f,  0.0f}, { 6.0f, 1.00f, 7.0f}, false},
    {{ 18.0f, -0.50f,  0.0f}, { 7.0f, 1.00f, 7.0f}, false},

    {{ 23.5f,  1.20f, -1.0f}, { 3.0f, 0.55f, 3.0f}, true },
    {{ 26.5f,  2.50f, -3.0f}, { 3.0f, 0.55f, 3.0f}, false},
    {{ 24.0f,  3.80f, -5.5f}, { 3.0f, 0.55f, 3.0f}, true },
    {{ 20.5f,  5.00f, -6.0f}, { 3.0f, 0.55f, 3.0f}, false},

    {{ 14.0f,  9.20f, -4.0f}, { 3.2f, 0.55f, 3.2f}, true },
    {{ 11.0f, 10.40f, -2.0f}, { 3.0f, 0.55f, 3.0f}, false},
    {{  8.0f, 11.60f,  0.0f}, { 3.0f, 0.55f, 3.0f}, true },

    {{ -2.5f, 14.10f, -0.5f}, { 3.0f, 0.55f, 3.0f}, false},
    {{ -5.5f, 15.30f, -2.5f}, { 3.2f, 0.55f, 3.2f}, true },
    {{ -8.5f, 16.50f, -4.0f}, { 4.4f, 0.70f, 4.4f}, false},
};

inline constexpr int kPlatformCount =
    static_cast<int>(sizeof(kPlatforms) / sizeof(kPlatforms[0]));

struct MovingPlatformDef {
    glm::vec3 from;
    glm::vec3 to;
    glm::vec3 size;
    float speed;
    float startOffset;
};

inline constexpr MovingPlatformDef kMovingPlatforms[] = {
    {{4.5f, 12.80f, 0.5f}, {0.5f, 12.80f, 0.5f}, {2.8f, 0.50f, 2.8f}, 2.4f, 0.0f},
};

inline constexpr int kMovingPlatformCount =
    static_cast<int>(sizeof(kMovingPlatforms) / sizeof(kMovingPlatforms[0]));

enum class EnemyKind { Walker, Spiker };

struct EnemyDef {
    EnemyKind kind;
    float minX;
    float maxX;
    float feetY;
    float z;
    float speed;
};

inline constexpr EnemyDef kEnemies[] = {
    {EnemyKind::Walker, -6.0f,  1.0f, 0.0f,  0.0f, 1.9f},
    {EnemyKind::Walker,  6.8f, 11.2f, 0.0f,  0.0f, 2.3f},
    {EnemyKind::Spiker, 15.3f, 20.7f, 0.0f,  0.0f, 1.6f},
    {EnemyKind::Walker, 19.4f, 21.6f, 5.275f, -6.0f, 1.7f},
    {EnemyKind::Walker,  6.9f,  9.1f, 11.875f, 0.0f, 1.8f},
};

inline constexpr int kEnemyCount =
    static_cast<int>(sizeof(kEnemies) / sizeof(kEnemies[0]));

inline constexpr glm::vec3 kItemBlocks[] = {
    { -1.00f, 3.20f, 0.0f},
    {  0.40f, 3.20f, 0.0f},
    {  9.00f, 3.20f, 0.0f},
    { 18.00f, 3.20f, 0.0f},
};

inline constexpr int kItemBlockCount =
    static_cast<int>(sizeof(kItemBlocks) / sizeof(kItemBlocks[0]));

struct BouncePadDef {
    glm::vec3 center;
    glm::vec3 size;
};

inline constexpr BouncePadDef kBouncePads[] = {
    {{17.0f, 5.60f, -5.0f}, {2.4f, 0.55f, 2.4f}},
};

inline constexpr int kBouncePadCount =
    static_cast<int>(sizeof(kBouncePads) / sizeof(kBouncePads[0]));

inline constexpr glm::vec3 kCoins[] = {
    {  3.90f, 1.80f,  0.0f},
    {  4.50f, 2.40f,  0.0f},
    {  5.10f, 1.80f,  0.0f},
    { 13.25f, 2.20f,  0.0f},
    { 23.50f, 2.40f, -1.0f},
    { 24.00f, 5.00f, -5.5f},
    { 14.00f, 10.60f,-4.0f},
    {  8.00f, 12.80f, 0.0f},
    { -5.50f, 16.70f,-2.5f},
};

inline constexpr int kCoinCount =
    static_cast<int>(sizeof(kCoins) / sizeof(kCoins[0]));

inline constexpr int kTotalCoins = kCoinCount + kItemBlockCount;

inline constexpr glm::vec3 kSpawnPoint {-7.0f, 0.70f, 0.0f};
inline constexpr glm::vec3 kGoalPoint  {-8.5f, 17.20f, -4.0f};
inline constexpr glm::vec3 kGoalSize   {1.8f, 0.70f, 1.8f};

inline constexpr float kKillPlaneY = -14.0f;

}
