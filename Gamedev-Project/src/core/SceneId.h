#pragma once

// The list of every screen the game can switch to, plus a helper that maps a
// floor index to its scene id for the menu, which thinks in floor numbers. Adding
// a screen means adding an entry here and the matching case in
// SceneManager::createScene().

enum class SceneId {
    None,
    LevelSelect,
    Floor1,
    Floor2,
    Floor3,
    Floor4,
};

inline SceneId sceneIdForFloor(int floorIndex) {
    switch (floorIndex) {
        case 0:  return SceneId::Floor1;
        case 1:  return SceneId::Floor2;
        case 2:  return SceneId::Floor3;
        case 3:  return SceneId::Floor4;
        default: return SceneId::LevelSelect;
    }
}
