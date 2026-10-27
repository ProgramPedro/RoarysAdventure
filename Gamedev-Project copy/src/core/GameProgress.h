#pragma once
//=============================================================================
// GameProgress - the shared save state: which floors are unlocked and which
// have been completed.
//
// ONE instance of this lives in main() and is handed to every scene through
// SceneContext as a reference, so there is a single source of truth. A level
// scene that the player beats calls markCompleted() on it, and the level-select
// screen sees the change the next time it draws.
//=============================================================================

#include "core/Config.h"

#include <array>

struct FloorState {
    bool unlocked  = false;
    bool completed = false;
};

class GameProgress {
public:
    static constexpr int kFloorCount = cfg::kFloorCount;

    // Starts a fresh game: floor 0 unlocked, everything else locked.
    GameProgress();

    bool isUnlocked(int floorIndex) const;
    bool isCompleted(int floorIndex) const;

    // Marks a floor finished and unlocks the one above it. This is the single
    // call a level scene makes when the player beats it.
    void markCompleted(int floorIndex);

    // Debug helper behind the U key on the menu: unlocks every floor so levels
    // can be tested without playing through the ones below.
    void unlockAll();

    // Back to a fresh game.
    void reset();

    int completedCount() const;

    // True once every floor is done - Pawlina is free.
    bool isGameComplete() const { return completedCount() == kFloorCount; }

private:
    static bool isValidIndex(int floorIndex);

    std::array<FloorState, kFloorCount> floors_{};
};
