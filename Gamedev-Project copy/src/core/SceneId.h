#pragma once
//=============================================================================
// SceneId - the list of every screen the game can switch to.
//
// TO ADD A NEW SCREEN: add an entry here, then add the matching case to
// SceneManager::createScene() in SceneManager.cpp. Those are the only two
// places that need to know the concrete scene type.
//=============================================================================

enum class SceneId {
    None,         // "no switch requested" - not a real scene
    LevelSelect,  // the tower menu, and the screen the game opens on
    Floor1,       // Mario-style platformer
    Floor2,       // "Run"-style shifting tunnel
    Floor3,       // stealth
    Floor4,       // boss fight against the rival mascots
};

// Maps a floor index (0-3) to its SceneId. Used by the menu, which thinks in
// floor numbers rather than scene names.
inline SceneId sceneIdForFloor(int floorIndex) {
    switch (floorIndex) {
        case 0:  return SceneId::Floor1;
        case 1:  return SceneId::Floor2;
        case 2:  return SceneId::Floor3;
        case 3:  return SceneId::Floor4;
        default: return SceneId::LevelSelect;
    }
}
