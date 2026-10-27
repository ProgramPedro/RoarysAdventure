#pragma once
//=============================================================================
// TextRenderer - draws screen-space text using the built-in 5x7 bitmap font.
//
// Coordinates are FRAMEBUFFER PIXELS with the origin at the TOP-LEFT, which is
// the usual convention for UI work: (0,0) is the top-left corner of the window
// and y grows downward.
//
// "scale" means how many screen pixels one font pixel becomes. At scale 4, a
// character is 20 pixels wide and 28 pixels tall.
//
// Every lit font pixel becomes a small quad. All the quads for one draw() call
// are batched into a single buffer upload and one draw call, so even a full
// screen of text is cheap.
//
// SWAPPING IN A REAL FONT LATER: replace the internals of this class with
// stb_truetype (bake an atlas in init, emit one textured quad per character in
// draw). The public interface below would not have to change, so no scene code
// would need editing.
//=============================================================================

#include "core/Shader.h"

#include <glm/vec3.hpp>
#include <string>
#include <vector>

class TextRenderer {
public:
    bool init();
    void cleanup();

    // Call once per frame before drawing text. Sets up the pixel-space
    // projection and turns off depth testing so text always sits on top.
    void begin(int framebufferWidth, int framebufferHeight);

    // Call when you are done with text for this frame, to put the GL state
    // back how the 3D rendering expects it.
    void end();

    // Draws text with its top-left corner at (x, y).
    void draw(const std::string& text, float x, float y, float scale,
              const glm::vec3& color, float alpha = 1.0f);

    // Draws text horizontally centered on centerX, top edge at y.
    void drawCentered(const std::string& text, float centerX, float y,
                      float scale, const glm::vec3& color, float alpha = 1.0f);

    // Fills a flat rectangle in the same pixel coordinate space. Handy for
    // panel backgrounds behind text. Must be called between begin() and end().
    void fillRect(float x, float y, float width, float height,
                  const glm::vec3& color, float alpha = 1.0f);

    // --- measuring (useful for centering and for laying out panels) ---------
    static float measureWidth(const std::string& text, float scale);
    static float measureHeight(float scale);

private:
    // Appends the six vertices of one axis-aligned quad to the batch.
    void pushQuad(float x, float y, float width, float height);

    // Uploads whatever is in batch_ and draws it in one call.
    void flush(const glm::vec3& color, float alpha);

    Shader shader_;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;

    // Reused between draw calls so we are not reallocating every frame.
    std::vector<float> batch_;
    std::size_t bufferCapacityBytes_ = 0;
};
