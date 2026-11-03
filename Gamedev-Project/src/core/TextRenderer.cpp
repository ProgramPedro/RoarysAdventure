#include "core/TextRenderer.h"

#include "core/BitmapFont.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cctype>

namespace {

// The text shader is as simple as it gets: take a 2D pixel coordinate, run it
// through an orthographic projection, and fill with a flat color.
constexpr const char* kTextVertexShader = R"GLSL(
#version 330 core
layout (location = 0) in vec2 aPixelPos;

uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPixelPos, 0.0, 1.0);
}
)GLSL";

constexpr const char* kTextFragmentShader = R"GLSL(
#version 330 core
out vec4 FragColor;

uniform vec3  uColor;
uniform float uAlpha;

void main() {
    FragColor = vec4(uColor, uAlpha);
}
)GLSL";

// One glyph advances this far horizontally, including the gap after it.
float advanceFor(float scale) {
    return static_cast<float>(font::kGlyphWidth + font::kGlyphGap) * scale;
}

} // namespace

bool TextRenderer::init() {
    if (!shader_.build(kTextVertexShader, kTextFragmentShader)) {
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Two floats per vertex: x and y in pixels.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return true;
}

void TextRenderer::cleanup() {
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    shader_.destroy();
    bufferCapacityBytes_ = 0;
}

void TextRenderer::begin(int framebufferWidth, int framebufferHeight) {
    // Text is an overlay: it must not be hidden by 3D geometry, and it needs
    // alpha blending so faded labels work.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Note the flipped top/bottom arguments: that is what puts (0,0) at the
    // TOP-left instead of OpenGL's usual bottom-left.
    const glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(std::max(framebufferWidth, 1)),
        static_cast<float>(std::max(framebufferHeight, 1)), 0.0f,
        -1.0f, 1.0f);

    shader_.use();
    shader_.setMat4("uProjection", projection);
}

void TextRenderer::end() {
    // Hand the depth test back to whoever draws 3D next frame.
    glEnable(GL_DEPTH_TEST);
}

void TextRenderer::pushQuad(float x, float y, float width, float height) {
    const float x0 = x;
    const float y0 = y;
    const float x1 = x + width;
    const float y1 = y + height;

    // Two triangles, six vertices. No index buffer: text batches are rebuilt
    // every frame and the duplicate vertices are not worth managing indices for.
    const float quad[12] = {
        x0, y0,  x1, y0,  x1, y1,
        x1, y1,  x0, y1,  x0, y0,
    };
    batch_.insert(batch_.end(), std::begin(quad), std::end(quad));
}

void TextRenderer::draw(const std::string& text, float x, float y, float scale,
                        const glm::vec3& color, float alpha) {
    batch_.clear();

    float cursorX = x;
    float cursorY = y;

    for (const char rawChar : text) {
        // Newlines let a single string span several lines.
        if (rawChar == '\n') {
            cursorX = x;
            cursorY += measureHeight(scale)
                     + static_cast<float>(font::kLineGap) * scale;
            continue;
        }

        // The font is uppercase-only, so fold lowercase input up to match.
        const char glyphChar = static_cast<char>(
            std::toupper(static_cast<unsigned char>(rawChar)));

        const font::Glyph* glyph = font::find(glyphChar);
        if (glyph == nullptr) {
            // Unknown character: leave a blank space rather than drawing junk.
            cursorX += advanceFor(scale);
            continue;
        }

        // Emit one quad per lit pixel in the 5x7 grid.
        for (int row = 0; row < font::kGlyphHeight; ++row) {
            const char* rowPixels = glyph->rows[row];
            for (int col = 0; col < font::kGlyphWidth; ++col) {
                if (rowPixels[col] != '#') continue;

                pushQuad(cursorX + static_cast<float>(col) * scale,
                         cursorY + static_cast<float>(row) * scale,
                         scale, scale);
            }
        }

        cursorX += advanceFor(scale);
    }

    flush(color, alpha);
}

void TextRenderer::fillRect(float x, float y, float width, float height,
                            const glm::vec3& color, float alpha) {
    batch_.clear();
    pushQuad(x, y, width, height);
    flush(color, alpha);
}

void TextRenderer::flush(const glm::vec3& color, float alpha) {
    if (batch_.empty()) {
        return;
    }

    shader_.setVec3("uColor", color);
    shader_.setFloat("uAlpha", alpha);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    const std::size_t bytes = batch_.size() * sizeof(float);
    if (bytes > bufferCapacityBytes_) {
        // Grow the buffer when a longer string comes along, then reuse it.
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes),
                     batch_.data(), GL_DYNAMIC_DRAW);
        bufferCapacityBytes_ = bytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes),
                        batch_.data());
    }

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch_.size() / 2));
    glBindVertexArray(0);
}

void TextRenderer::drawCentered(const std::string& text, float centerX, float y,
                                float scale, const glm::vec3& color, float alpha) {
    const float width = measureWidth(text, scale);
    draw(text, centerX - width * 0.5f, y, scale, color, alpha);
}

float TextRenderer::measureWidth(const std::string& text, float scale) {
    // Measure the widest line, so multi-line strings center correctly.
    float widest = 0.0f;
    float lineWidth = 0.0f;

    for (const char c : text) {
        if (c == '\n') {
            widest = std::max(widest, lineWidth);
            lineWidth = 0.0f;
            continue;
        }
        lineWidth += advanceFor(scale);
    }
    widest = std::max(widest, lineWidth);

    // The trailing gap after the final character is not part of the visible
    // width, so take it back off.
    if (widest > 0.0f) {
        widest -= static_cast<float>(font::kGlyphGap) * scale;
    }
    return widest;
}

float TextRenderer::measureHeight(float scale) {
    return static_cast<float>(font::kGlyphHeight) * scale;
}
