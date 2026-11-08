#pragma once

// The shared save state: which of the four floors are unlocked and which have
// been completed. One instance lives in main() and is handed to every scene by
// reference through SceneContext, so there is a single source of truth. A level
// calls markCompleted() when the player beats it.

#include "core/Config.h"

#include <array>

struct FloorState {
    bool unlocked  = false;
    bool completed = false;
};

class GameProgress {
public:
    static constexpr int kFloorCount = cfg::kFloorCount;

    GameProgress();

    bool isUnlocked(int floorIndex) const;
    bool isCompleted(int floorIndex) const;

    void markCompleted(int floorIndex);

    void unlockAll();

    void reset();

    int completedCount() const;

    bool isGameComplete() const { return completedCount() == kFloorCount; }

private:
    static bool isValidIndex(int floorIndex);

    std::array<FloorState, kFloorCount> floors_{};
};
