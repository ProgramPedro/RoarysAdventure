#pragma once

// Owns the shared GPU resources - the one lit shader, the primitive meshes, the
// background shader and the text renderer - and wraps them in short draw helpers
// so scene code reads like a description of the picture. There is exactly one
// Renderer for the whole game, created in main() and handed to every scene.

#include "core/Mesh.h"
#include "core/Shader.h"
#include "core/TextRenderer.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Renderer {
public:
    bool init();
    void cleanup();

    void beginFrame(int framebufferWidth, int framebufferHeight);

    void drawBackground(const glm::vec3& topColor,
                        const glm::vec3& bottomColor,
                        const glm::vec3& glowColor,
                        const glm::vec2& glowCenterNdc,
                        float glowRadius,
                        float glowStrength);

    void setCamera(const glm::mat4& view, const glm::mat4& projection);

    const glm::mat4& viewProjection() const { return viewProjection_; }

    void drawBox(const glm::mat4& model, const glm::vec3& color,
                 float alpha = 1.0f, float emissive = 0.0f);
    void drawPyramid(const glm::mat4& model, const glm::vec3& color,
                     float alpha = 1.0f, float emissive = 0.0f);
    void drawCylinder(const glm::mat4& model, const glm::vec3& color,
                      float alpha = 1.0f, float emissive = 0.0f);
    void drawSphere(const glm::mat4& model, const glm::vec3& color,
                    float alpha = 1.0f, float emissive = 0.0f);

    void drawBoxAt(const glm::vec3& center, const glm::vec3& size,
                   const glm::vec3& color, float alpha = 1.0f,
                   float emissive = 0.0f);
    void drawPyramidAt(const glm::vec3& center, const glm::vec3& size,
                       const glm::vec3& color, float alpha = 1.0f,
                       float emissive = 0.0f);
    void drawCylinderAt(const glm::vec3& center, const glm::vec3& size,
                        const glm::vec3& color, float alpha = 1.0f,
                        float emissive = 0.0f);
    void drawSphereAt(const glm::vec3& center, const glm::vec3& size,
                      const glm::vec3& color, float alpha = 1.0f,
                      float emissive = 0.0f);

    void setDepthWrite(bool enabled);

    TextRenderer& text() { return text_; }

private:
    void applyMaterial(const glm::mat4& model, const glm::vec3& color,
                       float alpha, float emissive);

    Shader litShader_;
    Shader backgroundShader_;

    Mesh boxMesh_;
    Mesh pyramidMesh_;
    Mesh cylinderMesh_;
    Mesh sphereMesh_;
    Mesh fullscreenQuad_;

    TextRenderer text_;

    glm::mat4 view_{1.0f};
    glm::mat4 projection_{1.0f};
    glm::mat4 viewProjection_{1.0f};
};

glm::mat4 makeTransform(const glm::vec3& center,
                        const glm::vec3& size,
                        const glm::vec3& eulerDegrees = glm::vec3(0.0f));
