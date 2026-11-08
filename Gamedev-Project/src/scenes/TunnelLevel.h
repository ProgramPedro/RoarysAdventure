#pragma once

// The layout of Floor 2: one row per tunnel segment, each listing the tiles on
// the four inner faces (floor, right, ceiling, left) as three lanes of '#' for a
// tile and '.' for a hole. Authoring it as text makes the whole run readable at a
// glance. Lane 2 of one face meets lane 0 of the next, which is how a path is
// routed from the floor onto a wall.

namespace level2 {
struct SegmentDef {
    const char* floorLanes;
    const char* rightLanes;
    const char* ceilingLanes;
    const char* leftLanes;
};

inline constexpr SegmentDef kSegments[] = {
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"##.", "...", "...", "..."},
    {"##.", "...", "...", "..."},
    {"###", "...", "...", "..."},

    {".##", "...", "...", "..."},
    {".##", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"#.#", "...", "...", "..."},
    {"#.#", "...", "...", "..."},
    {"###", "...", "...", "..."},

    {"..#", "###", "...", "..."},
    {"..#", "###", "...", "..."},
    {"...", "###", "...", "..."},
    {"...", "###", "...", "..."},
    {"...", "##.", "...", "..."},
    {"...", "##.", "...", "..."},
    {"...", "###", "...", "..."},
    {"..#", "###", "...", "..."},
    {"###", "#..", "...", "..."},
    {"###", "...", "...", "..."},

    {"###", "...", "...", "..."},
    {"##.", "...", "...", "..."},
    {".##", "###", "...", "..."},
    {"...", "###", "...", "..."},
    {"...", "###", "###", "..."},
    {"...", "..#", "###", "..."},
    {"...", "...", "###", "..."},
    {"...", "...", "###", "..."},
    {"...", "...", "##.", "..."},
    {"...", "...", "##.", "..."},
    {"...", "...", "###", "..."},
    {"...", "...", "###", "..."},

    {"...", "...", "..#", "###"},
    {"...", "...", "...", "###"},
    {"...", "...", "...", "###"},
    {"...", "...", "...", "##."},
    {"...", "...", "...", "###"},
    {"#..", "...", "...", "###"},
    {"###", "...", "...", "#.."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"#.#", "...", "...", "..."},
    {"#.#", "...", "...", "..."},
    {"###", "...", "...", "..."},

    {"###", "...", "...", "..."},
    {".#.", "...", "...", "..."},
    {".#.", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
    {"###", "...", "...", "..."},
};

inline constexpr int kSegmentCount =
    static_cast<int>(sizeof(kSegments) / sizeof(kSegments[0]));

inline constexpr const char* lanesFor(const SegmentDef& segment, int face) {
    return face == 0 ? segment.floorLanes
         : face == 1 ? segment.rightLanes
         : face == 2 ? segment.ceilingLanes
                     : segment.leftLanes;
}

inline constexpr bool hasTile(int segment, int face, int lane) {
    if (segment < 0 || segment >= kSegmentCount) return false;
    if (face < 0 || face > 3 || lane < 0 || lane > 2) return false;
    return lanesFor(kSegments[segment], face)[lane] == '#';
}

}
