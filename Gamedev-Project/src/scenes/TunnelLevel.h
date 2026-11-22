#pragma once
//=============================================================================
// TunnelLevel - THE LAYOUT OF FLOOR 2. EDIT THIS FILE TO REDESIGN IT.
//
// The tunnel is a square tube you auto-run down. Each row below is one segment
// of it, and describes which tiles exist on each of the four inner faces:
//
//        {  floor , right , ceiling, left  }
//
// Each face is three lanes wide, written left-to-right in that face's OWN
// frame, as if you were standing on it. The corners line up like this:
//
//        floor lane 2   <-->  right lane 0
//        right lane 2   <-->  ceiling lane 0
//        ceiling lane 2 <-->  left lane 0
//        left lane 2    <-->  floor lane 0
//
// So to send the player from the floor onto the right wall, leave floor lane 2
// solid, give the right wall a tile in lane 0, and then delete the floor.
//
//-----------------------------------------------------------------------------
// THE TILE ALPHABET
//
//   .   hole          nothing there. Run into one and the run is over.
//   #   solid         plain tile.
//   o   block         a solid tile with a crate standing on it. Strafe round
//                     it or jump it; walk into it at ground level and you wipe
//                     out. A crate is kBlockHeight tall and a jump clears it.
//   X   crumbling     solid when you arrive, gone kCrumbleDelay seconds after
//                     your feet touch it. It does not come back until the run
//                     restarts. Never stop on one.
//   ^   spring        launches you automatically the instant you land, far
//                     higher than a jump. The gaps after the springs in act 5
//                     are too wide to cross any other way.
//   >   speed         shoves you forward by kSpeedTileBoost, which then bleeds
//                     off over about a second and a half.
//   ~   ice           you still steer, but at kIceGrip of the usual rate, so
//                     a lane change takes about four times as long to start.
//
// Every one of those except '.' holds you up, so `hasTile` is simply "not a
// hole". Behaviour lives in TunnelScene; this file only says where things are.
//
//-----------------------------------------------------------------------------
// DESIGN RULES:
//   - You can shift about one lane per segment at running speed, so never
//     require a two-lane jog in a single row. Leave a clear row between.
//   - A jump clears roughly three segments; a spring clears about five.
//   - Every segment needs at least one reachable tile somewhere, or the run
//     is impossible. tools/check_tunnel_solvable.py checks this, and also
//     checks that each gap after a spring is actually crossable.
//   - Each act introduces ONE idea, practises it, then tests it. Keep that
//     shape if you add an act: it is what makes the level teachable.
//=============================================================================

namespace level2 {

//-----------------------------------------------------------------------------
// The tile alphabet, as named constants so scene code never compares to a
// bare character literal.
//-----------------------------------------------------------------------------
inline constexpr char kEmpty   = '.';
inline constexpr char kSolid   = '#';
inline constexpr char kBlock   = 'o';
inline constexpr char kCrumble = 'X';
inline constexpr char kSpring  = '^';
inline constexpr char kSpeed   = '>';
inline constexpr char kIce     = '~';

struct SegmentDef {
    const char* floorLanes;
    const char* rightLanes;
    const char* ceilingLanes;
    const char* leftLanes;
};

inline constexpr SegmentDef kSegments[] = {
    //------------------------------------------------------------------------
    // ACT 1  (0-19)  -  LEARN TO RUN
    //
    // Open floor, then holes that drift from lane to lane. Nothing here can
    // kill you except running into a gap, so this is where a new player finds
    // out that A and D move them and that the floor is three lanes wide.
    //------------------------------------------------------------------------
    {"###", "...", "...", "..."},   //   0  << checkpoint
    {"###", "...", "...", "..."},   //   1
    {"###", "...", "...", "..."},   //   2
    {"###", "...", "...", "..."},   //   3
    {"###", "...", "...", "..."},   //   4
    {"##.", "...", "...", "..."},   //   5
    {"##.", "...", "...", "..."},   //   6
    {"###", "...", "...", "..."},   //   7
    {".##", "...", "...", "..."},   //   8
    {".##", "...", "...", "..."},   //   9
    {"###", "...", "...", "..."},   //  10
    {"#.#", "...", "...", "..."},   //  11
    {"#.#", "...", "...", "..."},   //  12
    {"###", "...", "...", "..."},   //  13
    {"##.", "...", "...", "..."},   //  14
    {".##", "...", "...", "..."},   //  15
    {"###", "...", "...", "..."},   //  16
    {"#.#", "...", "...", "..."},   //  17
    {"#.#", "...", "...", "..."},   //  18
    {"###", "...", "...", "..."},   //  19

    //------------------------------------------------------------------------
    // ACT 2  (20-43)  -  CRATES
    //
    // 'o' is a solid tile with a crate standing on it. Introduced one at a
    // time in the middle, then at the edges, then in pairs that leave exactly
    // one lane open. The last row is all three blocked: the only way past is
    // to jump, which is the test.
    //------------------------------------------------------------------------
    {"###", "...", "...", "..."},   //  20  << checkpoint
    {"###", "...", "...", "..."},   //  21
    {"#o#", "...", "...", "..."},   //  22
    {"###", "...", "...", "..."},   //  23
    {"#o#", "...", "...", "..."},   //  24
    {"###", "...", "...", "..."},   //  25
    {"o##", "...", "...", "..."},   //  26
    {"###", "...", "...", "..."},   //  27
    {"##o", "...", "...", "..."},   //  28
    {"###", "...", "...", "..."},   //  29
    {"#o#", "...", "...", "..."},   //  30
    {"#o#", "...", "...", "..."},   //  31
    {"###", "...", "...", "..."},   //  32
    {"o#o", "...", "...", "..."},   //  33
    {"o#o", "...", "...", "..."},   //  34
    {"###", "...", "...", "..."},   //  35
    {"#oo", "...", "...", "..."},   //  36
    {"###", "...", "...", "..."},   //  37
    {"###", "...", "...", "..."},   //  38
    {"oo#", "...", "...", "..."},   //  39
    {"###", "...", "...", "..."},   //  40
    {"o#o", "...", "...", "..."},   //  41
    {"###", "...", "...", "..."},   //  42
    {"ooo", "...", "...", "..."},   //  43

    //------------------------------------------------------------------------
    // ACT 3  (44-69)  -  THE WALLS
    //
    // The floor runs out and the right wall takes over, then the ceiling.
    // Crates come along for the ride, so by the time you are upside down you
    // are dodging things you already know while gravity has moved.
    //------------------------------------------------------------------------
    {"###", "...", "...", "..."},   //  44  << checkpoint
    {"###", "...", "...", "..."},   //  45
    {"..#", "###", "...", "..."},   //  46
    {"..#", "###", "...", "..."},   //  47
    {"...", "###", "...", "..."},   //  48
    {"...", "###", "...", "..."},   //  49
    {"...", "##.", "...", "..."},   //  50
    {"...", "##.", "...", "..."},   //  51
    {"...", "###", "...", "..."},   //  52
    {"...", "#o#", "...", "..."},   //  53
    {"...", "###", "...", "..."},   //  54
    {"...", "o#o", "...", "..."},   //  55
    {"...", "###", "...", "..."},   //  56
    {"..#", "###", "...", "..."},   //  57
    {"###", "#..", "...", "..."},   //  58
    {"###", "...", "...", "..."},   //  59
    {"###", "...", "...", "..."},   //  60
    {"##.", "...", "...", "..."},   //  61
    {".##", "###", "...", "..."},   //  62
    {"...", "###", "...", "..."},   //  63
    {"...", "###", "###", "..."},   //  64
    {"...", "..#", "###", "..."},   //  65
    {"...", "...", "###", "..."},   //  66
    {"...", "...", "##.", "..."},   //  67
    {"...", "...", "###", "..."},   //  68
    {"...", "...", "#o#", "..."},   //  69

    //------------------------------------------------------------------------
    // ACT 4  (70-95)  -  CRUMBLING TILES
    //
    // Down the far wall back to the floor, then 'X': a tile that holds for a
    // moment after you touch it and then drops away. Standing still is the
    // one thing that kills you, so the lesson is 'keep moving'. The last row
    // mixes a crumbling lane with a crate, so neither option is free.
    //------------------------------------------------------------------------
    {"...", "...", "###", "..."},   //  70  << checkpoint
    {"...", "...", "..#", "###"},   //  71
    {"...", "...", "...", "###"},   //  72
    {"#..", "...", "...", "###"},   //  73
    {"###", "...", "...", "#.."},   //  74
    {"###", "...", "...", "..."},   //  75
    {"###", "...", "...", "..."},   //  76
    {"#X#", "...", "...", "..."},   //  77
    {"###", "...", "...", "..."},   //  78
    {"#X#", "...", "...", "..."},   //  79
    {"###", "...", "...", "..."},   //  80
    {"XXX", "...", "...", "..."},   //  81
    {"###", "...", "...", "..."},   //  82
    {"XX#", "...", "...", "..."},   //  83
    {"#XX", "...", "...", "..."},   //  84
    {"###", "...", "...", "..."},   //  85
    {"X#X", "...", "...", "..."},   //  86
    {"###", "...", "...", "..."},   //  87
    {"XXX", "...", "...", "..."},   //  88
    {"XXX", "...", "...", "..."},   //  89
    {"###", "...", "...", "..."},   //  90
    {"X.X", "...", "...", "..."},   //  91
    {"###", "...", "...", "..."},   //  92
    {"XoX", "...", "...", "..."},   //  93
    {"###", "...", "...", "..."},   //  94
    {"###", "...", "...", "..."},   //  95

    //------------------------------------------------------------------------
    // ACT 5  (96-127)  -  SPRINGS
    //
    // '^' throws you much higher than a jump does, and every gap after one is
    // three segments wide - a plain jump covers two, so the spring is the
    // only way across. Full width first so it cannot be missed, then narrowed
    // to a single lane you have to line up with.
    //------------------------------------------------------------------------
    {"###", "...", "...", "..."},   //  96  << checkpoint
    {"###", "...", "...", "..."},   //  97
    {"^^^", "...", "...", "..."},   //  98
    {"...", "...", "...", "..."},   //  99
    {"...", "...", "...", "..."},   // 100
    {"...", "...", "...", "..."},   // 101
    {"###", "...", "...", "..."},   // 102
    {"###", "...", "...", "..."},   // 103
    {"^^#", "...", "...", "..."},   // 104
    {"...", "...", "...", "..."},   // 105
    {"...", "...", "...", "..."},   // 106
    {"...", "...", "...", "..."},   // 107
    {"###", "...", "...", "..."},   // 108
    {"###", "...", "...", "..."},   // 109
    {"#^^", "...", "...", "..."},   // 110
    {"...", "...", "...", "..."},   // 111
    {"...", "...", "...", "..."},   // 112
    {"...", "...", "...", "..."},   // 113
    {"###", "...", "...", "..."},   // 114
    {"###", "...", "...", "..."},   // 115
    {"#^#", "...", "...", "..."},   // 116
    {"...", "...", "...", "..."},   // 117
    {"...", "...", "...", "..."},   // 118
    {"...", "...", "...", "..."},   // 119
    {"###", "...", "...", "..."},   // 120
    {"###", "...", "...", "..."},   // 121
    {"#^#", "...", "...", "..."},   // 122
    {"...", "...", "...", "..."},   // 123
    {"...", "...", "...", "..."},   // 124
    {"...", "...", "...", "..."},   // 125
    {"###", "...", "...", "..."},   // 126
    {"###", "...", "...", "..."},   // 127

    //------------------------------------------------------------------------
    // ACT 6  (128-155)  -  SPEED AND ICE
    //
    // '>' shoves you forward for about a second and a half. '~' is ice: you
    // still steer, but it takes roughly four segments to complete a lane
    // change instead of one. Ice is introduced with nothing to do but hold
    // straight, then with a safe middle lane, then with the one hole that
    // forces a turn - and the four icy rows before it are exactly the run-up
    // that turn needs. The last stretch is a boost straight onto ice.
    //------------------------------------------------------------------------
    {"###", "...", "...", "..."},   // 128  << checkpoint
    {"#>#", "...", "...", "..."},   // 129
    {"###", "...", "...", "..."},   // 130
    {"###", "...", "...", "..."},   // 131
    {">>>", "...", "...", "..."},   // 132
    {"...", "...", "...", "..."},   // 133
    {"...", "...", "...", "..."},   // 134
    {"###", "...", "...", "..."},   // 135
    {"###", "...", "...", "..."},   // 136
    {"~~~", "...", "...", "..."},   // 137
    {"~~~", "...", "...", "..."},   // 138
    {"~~~", "...", "...", "..."},   // 139
    {"###", "...", "...", "..."},   // 140
    {"~#~", "...", "...", "..."},   // 141
    {"~#~", "...", "...", "..."},   // 142
    {"###", "...", "...", "..."},   // 143
    {"~~~", "...", "...", "..."},   // 144
    {"~~~", "...", "...", "..."},   // 145
    {"~~~", "...", "...", "..."},   // 146
    {"~~~", "...", "...", "..."},   // 147
    {"~.~", "...", "...", "..."},   // 148
    {"~~~", "...", "...", "..."},   // 149
    {"###", "...", "...", "..."},   // 150
    {">>>", "...", "...", "..."},   // 151
    {"~~~", "...", "...", "..."},   // 152
    {"~~~", "...", "...", "..."},   // 153
    {"###", "...", "...", "..."},   // 154
    {"###", "...", "...", "..."},   // 155

    //------------------------------------------------------------------------
    // ACT 7  (156-179)  -  THE GAUNTLET
    //
    // All the way round the tunnel with everything mixed in: crumbling tiles
    // on a wall, a crate on the ceiling, ice on the far wall. Nothing new is
    // introduced here - it is the exam.
    //------------------------------------------------------------------------
    {"..#", "###", "...", "..."},   // 156  << checkpoint
    {"...", "###", "...", "..."},   // 157
    {"...", "#X#", "...", "..."},   // 158
    {"...", "###", "...", "..."},   // 159
    {"...", "#o#", "...", "..."},   // 160
    {"...", "###", "###", "..."},   // 161
    {"...", "..#", "###", "..."},   // 162
    {"...", "...", "#X#", "..."},   // 163
    {"...", "...", "###", "..."},   // 164
    {"...", "...", "##.", "..."},   // 165
    {"...", "...", "###", "..."},   // 166
    {"...", "...", "XoX", "..."},   // 167
    {"...", "...", "###", "..."},   // 168
    {"...", "...", "..#", "###"},   // 169
    {"...", "...", "...", "###"},   // 170
    {"...", "...", "...", "#o#"},   // 171
    {"...", "...", "...", "###"},   // 172
    {"...", "...", "...", "~~~"},   // 173
    {"...", "...", "...", "###"},   // 174
    {"#..", "...", "...", "###"},   // 175
    {"###", "...", "...", "#.."},   // 176
    {"###", "...", "...", "..."},   // 177
    {"#X#", "...", "...", "..."},   // 178
    {"###", "...", "...", "..."},   // 179

    //------------------------------------------------------------------------
    // ACT 8  (180-193)  -  THE RUN IN
    //
    // A speed tile into the final stretch: a split, a one-lane beam, a last
    // crate and a last crumbling row, all taken faster than anything before.
    //------------------------------------------------------------------------
    {"###", "...", "...", "..."},   // 180  << checkpoint
    {">>>", "...", "...", "..."},   // 181
    {"#.#", "...", "...", "..."},   // 182
    {"#.#", "...", "...", "..."},   // 183
    {"###", "...", "...", "..."},   // 184
    {".#.", "...", "...", "..."},   // 185
    {".#.", "...", "...", "..."},   // 186
    {"###", "...", "...", "..."},   // 187
    {"#o#", "...", "...", "..."},   // 188
    {"###", "...", "...", "..."},   // 189
    {"XXX", "...", "...", "..."},   // 190
    {"###", "...", "...", "..."},   // 191
    {"###", "...", "...", "..."},   // 192
    {"###", "...", "...", "..."},   // 193
};

inline constexpr int kSegmentCount =
    static_cast<int>(sizeof(kSegments) / sizeof(kSegments[0]));

//-----------------------------------------------------------------------------
// CHECKPOINTS - the segment each act begins at.
//
// Whether a wipeout sends you back to the last one of these or all the way to
// the start is cfg::tunnel::kUseCheckpoints. It ships OFF, because restarting
// the whole run is how the games this is based on do it and it is what keeps
// the pressure on. If playtesting says the tunnel is now too long to learn
// that way, flipping that one bool is the fix.
//-----------------------------------------------------------------------------
inline constexpr int kCheckpoints[] = {0, 20, 44, 70, 96, 128, 156, 180};

inline constexpr int kCheckpointCount =
    static_cast<int>(sizeof(kCheckpoints) / sizeof(kCheckpoints[0]));

// The last checkpoint at or before `segment`.
inline constexpr int checkpointBefore(int segment) {
    int best = 0;
    for (int i = 0; i < kCheckpointCount; ++i) {
        if (kCheckpoints[i] <= segment) best = kCheckpoints[i];
    }
    return best;
}

//-----------------------------------------------------------------------------
// THE ACTS - what each stretch of the tunnel is teaching.
//
// Shown on screen while you run, so a player always knows what the current
// section is about. Each act starts at the checkpoint of the same index.
//-----------------------------------------------------------------------------
struct ActDef {
    int firstSegment;
    const char* name;
};

inline constexpr ActDef kActs[] = {
    {  0, "LEARN TO RUN"   },
    { 20, "CRATES"         },
    { 44, "THE WALLS"      },
    { 70, "CRUMBLING TILES"},
    { 96, "SPRINGS"        },
    {128, "SPEED AND ICE"  },
    {156, "THE GAUNTLET"   },
    {180, "THE RUN IN"     },
};

inline constexpr int kActCount =
    static_cast<int>(sizeof(kActs) / sizeof(kActs[0]));

inline constexpr const char* actNameFor(int segment) {
    const char* name = kActs[0].name;
    for (int i = 0; i < kActCount; ++i) {
        if (kActs[i].firstSegment <= segment) name = kActs[i].name;
    }
    return name;
}

//-----------------------------------------------------------------------------
// Reading the tables
//-----------------------------------------------------------------------------

// Which face a segment's lane string belongs to, so the scene can loop.
inline constexpr const char* lanesFor(const SegmentDef& segment, int face) {
    return face == 0 ? segment.floorLanes
         : face == 1 ? segment.rightLanes
         : face == 2 ? segment.ceilingLanes
                     : segment.leftLanes;
}

// The tile glyph at a position, or kEmpty for anything out of bounds.
inline constexpr char tileAt(int segment, int face, int lane) {
    if (segment < 0 || segment >= kSegmentCount) return kEmpty;
    if (face < 0 || face > 3 || lane < 0 || lane > 2) return kEmpty;
    return lanesFor(kSegments[segment], face)[lane];
}

// Is there something to stand on here? Every glyph except a hole holds you up.
inline constexpr bool hasTile(int segment, int face, int lane) {
    return tileAt(segment, face, lane) != kEmpty;
}

} // namespace level2
