#pragma once
//=============================================================================
// TunnelLevel - THE LAYOUT OF FLOOR 2. EDIT THIS FILE TO REDESIGN IT.
//
// The tunnel is a square tube you auto-run down. Each row below is one segment
// of it, and describes which tiles exist on each of the four inner faces:
//
//        {  floor , right , ceiling, left  }
//
// Each face is three lanes wide. '#' is a tile, '.' is a hole.
//
// WHICH LANE IS WHICH: lanes read left-to-right in that face's OWN frame, as
// if you were standing on it. The corners line up like this:
//
//        floor lane 2  <-->  right lane 0
//        right lane 2  <-->  ceiling lane 0
//        ceiling lane 2 <--> left lane 0
//        left lane 2   <-->  floor lane 0
//
// So to send the player from the floor onto the right wall, leave floor lane 2
// solid, give the right wall a tile in lane 0, and then delete the floor.
//
// DESIGN RULES:
//   - You can shift about one lane per segment at running speed, so never
//     require a two-lane jog in a single row.
//   - A jump clears roughly two segments.
//   - Every segment needs at least one reachable tile somewhere, or the run
//     is impossible. There is a solvability checker for this - see the README.
//=============================================================================

namespace level2 {

struct SegmentDef {
    const char* floorLanes;
    const char* rightLanes;
    const char* ceilingLanes;
    const char* leftLanes;
};

inline constexpr SegmentDef kSegments[] = {
    // --- 0-7: open running, learn the controls -----------------------------
    {"###", "...", "...", "..."},   // 0
    {"###", "...", "...", "..."},   // 1
    {"###", "...", "...", "..."},   // 2
    {"###", "...", "...", "..."},   // 3
    {"###", "...", "...", "..."},   // 4
    {"##.", "...", "...", "..."},   // 5   first hole, on the right
    {"##.", "...", "...", "..."},   // 6
    {"###", "...", "...", "..."},   // 7

    // --- 8-13: dodging between lanes ---------------------------------------
    {".##", "...", "...", "..."},   // 8
    {".##", "...", "...", "..."},   // 9
    {"###", "...", "...", "..."},   // 10
    {"#.#", "...", "...", "..."},   // 11  split down the middle
    {"#.#", "...", "...", "..."},   // 12  << checkpoint
    {"###", "...", "...", "..."},   // 13

    // --- 14-23: the first wall. The floor runs out, so you have to take it --
    {"..#", "###", "...", "..."},   // 14  floor narrows to the wall side
    {"..#", "###", "...", "..."},   // 15
    {"...", "###", "...", "..."},   // 16  no floor at all now
    {"...", "###", "...", "..."},   // 17
    {"...", "##.", "...", "..."},   // 18  holes in the wall
    {"...", "##.", "...", "..."},   // 19
    {"...", "###", "...", "..."},   // 20
    {"..#", "###", "...", "..."},   // 21  floor comes back
    {"###", "#..", "...", "..."},   // 22
    {"###", "...", "...", "..."},   // 23

    // --- 24-35: all the way round to the ceiling ---------------------------
    {"###", "...", "...", "..."},   // 24  << checkpoint
    {"##.", "...", "...", "..."},   // 25
    {".##", "###", "...", "..."},   // 26  back onto the wall
    {"...", "###", "...", "..."},   // 27
    {"...", "###", "###", "..."},   // 28  the ceiling opens up
    {"...", "..#", "###", "..."},   // 29  wall runs out at the top
    {"...", "...", "###", "..."},   // 30  upside down
    {"...", "...", "###", "..."},   // 31
    {"...", "...", "##.", "..."},   // 32
    {"...", "...", "##.", "..."},   // 33
    {"...", "...", "###", "..."},   // 34
    {"...", "...", "###", "..."},   // 35

    // --- 36-47: down the far wall and back to the floor --------------------
    {"...", "...", "..#", "###"},   // 36  << checkpoint
    {"...", "...", "...", "###"},   // 37
    {"...", "...", "...", "###"},   // 38
    {"...", "...", "...", "##."},   // 39
    {"...", "...", "...", "###"},   // 40
    {"#..", "...", "...", "###"},   // 41  floor again, on the far side
    {"###", "...", "...", "#.."},   // 42
    {"###", "...", "...", "..."},   // 43
    {"###", "...", "...", "..."},   // 44
    {"#.#", "...", "...", "..."},   // 45
    {"#.#", "...", "...", "..."},   // 46
    {"###", "...", "...", "..."},   // 47

    // --- 48-55: the run in ---------------------------------------------------
    {"###", "...", "...", "..."},   // 48  << checkpoint
    {".#.", "...", "...", "..."},   // 49  narrow beam
    {".#.", "...", "...", "..."},   // 50
    {"###", "...", "...", "..."},   // 51
    {"###", "...", "...", "..."},   // 52
    {"###", "...", "...", "..."},   // 53
    {"###", "...", "...", "..."},   // 54
    {"###", "...", "...", "..."},   // 55  finish line
};

inline constexpr int kSegmentCount =
    static_cast<int>(sizeof(kSegments) / sizeof(kSegments[0]));

// Which face a segment's lane string belongs to, so the scene can loop.
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

} // namespace level2
