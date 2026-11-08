#pragma once

// Draws screen-space text and solid UI rectangles using the built-in 5x7 bitmap
// font. Coordinates are framebuffer pixels with the origin at the top-left, and
// "scale" is how many screen pixels one font pixel becomes. Swapping in a real
// font later would not change this interface, so no scene code would need edits.

#include "core/Shader.h"

#include <glm/vec3.hpp>
#include <string>
#include <vector>

class TextRenderer {
public:
    bool init();
    void cleanup();

    void begin(int framebufferWidth, int framebufferHeight);

    void end();

    void draw(const std::string& text, float x, float y, float scale,
              const glm::vec3& color, float alpha = 1.0f);

    void drawCentered(const std::string& text, float centerX, float y,
                      float scale, const glm::vec3& color, float alpha = 1.0f);

    void fillRect(float x, float y, float width, float height,
                  const glm::vec3& color, float alpha = 1.0f);

    static float measureWidth(const std::string& text, float scale);
    static float measureHeight(float scale);

private:
    void pushQuad(float x, float y, float width, float height);

    void flush(const glm::vec3& color, float alpha);

    Shader shader_;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;

    std::vector<float> batch_;
    std::size_t bufferCapacityBytes_ = 0;
};
