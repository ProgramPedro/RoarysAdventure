#pragma once
//=============================================================================
// Screenshot - saves the current framebuffer to a PPM image file.
//
// This is a development convenience, not part of the game: it lets you grab
// clean images of a scene for the report without a screen-capture tool, and it
// is how the menu layout can be checked automatically.
//
// PPM is used because it is about ten lines of code and needs no image
// library. Preview, GIMP and most tools open it directly; macOS `sips` and
// ImageMagick can convert it to PNG.
//
// Run the game with:  ./roary --screenshot shot.ppm
//=============================================================================

#include <string>

namespace screenshot {

// Reads the pixels currently in the back buffer and writes them to `path`.
// Call it right after rendering a frame and before swapping buffers.
// Returns false (and explains why on stderr) if the file could not be written.
bool saveFramebuffer(const std::string& path, int width, int height);

} // namespace screenshot
