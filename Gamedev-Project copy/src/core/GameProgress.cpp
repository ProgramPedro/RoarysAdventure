#include "core/GameProgress.h"

#include <iostream>

GameProgress::GameProgress() {
    reset();
}

bool GameProgress::isValidIndex(int floorIndex) {
    return floorIndex >= 0 && floorIndex < kFloorCount;
}

void GameProgress::reset() {
    for (auto& floor : floors_) {
        floor.unlocked = false;
        floor.completed = false;
    }
    // The bottom floor is always available at the start of a new game.
    floors_[0].unlocked = true;
}

bool GameProgress::isUnlocked(int floorIndex) const {
    if (!isValidIndex(floorIndex)) return false;
    return floors_[static_cast<std::size_t>(floorIndex)].unlocked;
}

bool GameProgress::isCompleted(int floorIndex) const {
    if (!isValidIndex(floorIndex)) return false;
    return floors_[static_cast<std::size_t>(floorIndex)].completed;
}

void GameProgress::markCompleted(int floorIndex) {
    if (!isValidIndex(floorIndex)) return;

    floors_[static_cast<std::size_t>(floorIndex)].completed = true;

    // Beating a floor opens the next one up. The top floor has nothing above
    // it, so the bounds check quietly handles that case.
    const int nextFloor = floorIndex + 1;
    if (isValidIndex(nextFloor)) {
        floors_[static_cast<std::size_t>(nextFloor)].unlocked = true;
    }

    std::cout << "[Progress] Floor " << (floorIndex + 1) << " complete. ("
              << completedCount() << "/" << kFloorCount << ")\n";
}

void GameProgress::unlockAll() {
    for (auto& floor : floors_) {
        floor.unlocked = true;
    }
    std::cout << "[Progress] DEBUG: all floors unlocked.\n";
}

int GameProgress::completedCount() const {
    int count = 0;
    for (const auto& floor : floors_) {
        if (floor.completed) ++count;
    }
    return count;
}
