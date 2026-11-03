#pragma once
//=============================================================================
// BitmapFont - a tiny 5x7 pixel font built right into the source.
//
// WHY THIS EXISTS: the project needs on-screen text (a title, floor names,
// descriptions) but we did not want to add a font library or ship a .ttf file
// for a class project. Each glyph is spelled out below as seven 5-character
// strings, where '#' is a lit pixel and ' ' is empty. That makes the font
// trivial to read and edit by hand.
//
// The font is UPPERCASE ONLY. TextRenderer uppercases whatever you give it, so
// lowercase strings still draw correctly.
//
// TO ADD A CHARACTER: add an entry to kGlyphs with its 7 rows. To replace this
// whole font with real anti-aliased type later, swap in stb_truetype behind
// TextRenderer's interface; nothing outside TextRenderer.cpp uses this file.
//=============================================================================

namespace font {

inline constexpr int kGlyphWidth  = 5;  // pixels across
inline constexpr int kGlyphHeight = 7;  // pixels down
inline constexpr int kGlyphGap    = 1;  // blank pixel columns between letters
inline constexpr int kLineGap     = 2;  // blank pixel rows between lines

struct Glyph {
    char character;
    const char* rows[kGlyphHeight];
};

inline constexpr Glyph kGlyphs[] = {
    {' ', {"     ","     ","     ","     ","     ","     ","     "}},

    {'A', {" ### ","#   #","#   #","#####","#   #","#   #","#   #"}},
    {'B', {"#### ","#   #","#   #","#### ","#   #","#   #","#### "}},
    {'C', {" ### ","#   #","#    ","#    ","#    ","#   #"," ### "}},
    {'D', {"#### ","#   #","#   #","#   #","#   #","#   #","#### "}},
    {'E', {"#####","#    ","#    ","#### ","#    ","#    ","#####"}},
    {'F', {"#####","#    ","#    ","#### ","#    ","#    ","#    "}},
    {'G', {" ### ","#   #","#    ","#  ##","#   #","#   #"," ### "}},
    {'H', {"#   #","#   #","#   #","#####","#   #","#   #","#   #"}},
    {'I', {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","#####"}},
    {'J', {"#####","   # ","   # ","   # ","   # ","#  # "," ##  "}},
    {'K', {"#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #"}},
    {'L', {"#    ","#    ","#    ","#    ","#    ","#    ","#####"}},
    {'M', {"#   #","## ##","# # #","#   #","#   #","#   #","#   #"}},
    {'N', {"#   #","##  #","# # #","#  ##","#   #","#   #","#   #"}},
    {'O', {" ### ","#   #","#   #","#   #","#   #","#   #"," ### "}},
    {'P', {"#### ","#   #","#   #","#### ","#    ","#    ","#    "}},
    {'Q', {" ### ","#   #","#   #","#   #","# # #","#  # "," ## #"}},
    {'R', {"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"}},
    {'S', {" ####","#    ","#    "," ### ","    #","    #","#### "}},
    {'T', {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "}},
    {'U', {"#   #","#   #","#   #","#   #","#   #","#   #"," ### "}},
    {'V', {"#   #","#   #","#   #","#   #","#   #"," # # ","  #  "}},
    {'W', {"#   #","#   #","#   #","#   #","# # #","## ##","#   #"}},
    {'X', {"#   #","#   #"," # # ","  #  "," # # ","#   #","#   #"}},
    {'Y', {"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "}},
    {'Z', {"#####","    #","   # ","  #  "," #   ","#    ","#####"}},

    {'0', {" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "}},
    {'1', {"  #  "," ##  ","  #  ","  #  ","  #  ","  #  ","#####"}},
    {'2', {" ### ","#   #","    #","   # ","  #  "," #   ","#####"}},
    {'3', {"#####","   # ","  #  ","   # ","    #","#   #"," ### "}},
    {'4', {"   # ","  ## "," # # ","#  # ","#####","   # ","   # "}},
    {'5', {"#####","#    ","#### ","    #","    #","#   #"," ### "}},
    {'6', {"  ## "," #   ","#    ","#### ","#   #","#   #"," ### "}},
    {'7', {"#####","    #","   # ","  #  "," #   "," #   "," #   "}},
    {'8', {" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "}},
    {'9', {" ### ","#   #","#   #"," ####","    #","   # "," ##  "}},

    {'.', {"     ","     ","     ","     ","     "," ##  "," ##  "}},
    {',', {"     ","     ","     ","     "," ##  ","  #  "," #   "}},
    {'!', {"  #  ","  #  ","  #  ","  #  ","  #  ","     ","  #  "}},
    {'?', {" ### ","#   #","    #","   # ","  #  ","     ","  #  "}},
    {'\'',{"  #  ","  #  ","     ","     ","     ","     ","     "}},
    {':', {"     "," ##  "," ##  ","     "," ##  "," ##  ","     "}},
    {'-', {"     ","     ","     ","#####","     ","     ","     "}},
    {'/', {"    #","    #","   # ","  #  "," #   ","#    ","#    "}},
    {'(', {"   # ","  #  "," #   "," #   "," #   ","  #  ","   # "}},
    {')', {" #   ","  #  ","   # ","   # ","   # ","  #  "," #   "}},
    {'[', {" ### "," #   "," #   "," #   "," #   "," #   "," ### "}},
    {']', {" ### ","   # ","   # ","   # ","   # ","   # "," ### "}},
    {'+', {"     ","  #  ","  #  ","#####","  #  ","  #  ","     "}},
    {'*', {"     ","# # #"," ### ","#####"," ### ","# # #","     "}},
    {'<', {"    #","   # ","  #  "," #   ","  #  ","   # ","    #"}},
    {'>', {"#    "," #   ","  #  ","   # ","  #  "," #   ","#    "}},
    {'_', {"     ","     ","     ","     ","     ","     ","#####"}},
    {'=', {"     ","     ","#####","     ","#####","     ","     "}},
};

inline constexpr int kGlyphCount =
    static_cast<int>(sizeof(kGlyphs) / sizeof(kGlyphs[0]));

// Finds a glyph by character, or returns nullptr if we have no artwork for it.
inline const Glyph* find(char c) {
    for (int i = 0; i < kGlyphCount; ++i) {
        if (kGlyphs[i].character == c) {
            return &kGlyphs[i];
        }
    }
    return nullptr;
}

} // namespace font
