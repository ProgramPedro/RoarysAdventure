#pragma once
//=============================================================================
// Renderer - owns the shared GPU resources (one lit shader, the primitive
// meshes, the background shader, the text renderer) and exposes short helpers
// so scene code reads like a description of the picture rather than a pile of
// OpenGL calls.
//
// There is exactly ONE Renderer for the whole game. It is created in main()
// and handed to every scene through SceneContext, so scenes never create or
// destroy GPU resources of their own unless they really need to.
//=============================================================================

#include "core/Mesh.h"
#include "core/Shader.h"
#include "core/TextRenderer.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Renderer {
public:
    bool init();
    void cleanup();

    // Sets the GL viewport and clears depth. The background draw below paints
    // over every pixel, so there is no need to clear color as well.
    void beginFrame(int framebufferWidth, int framebufferHeight);

    //-------------------------------------------------------------------------
    // Background: a vertical gradient plus a soft radial glow. This is the
    // FIU blue-and-gold backdrop the tower sits in front of.
    //-------------------------------------------------------------------------
    void drawBackground(const glm::vec3& topColor,
                        const glm::vec3& bottomColor,
                        const glm::vec3& glowColor,
                        const glm::vec2& glowCenterNdc,
                        float glowRadius,
                        float glowStrength);

    //-------------------------------------------------------------------------
    // 3D drawing. Call setCamera() once per frame, then draw shapes.
    //-------------------------------------------------------------------------
    void setCamera(const glm::mat4& view, const glm::mat4& projection);

    // The combined matrix, needed for projecting world points to the screen.
    const glm::mat4& viewProjection() const { return viewProjection_; }

    // emissive = 0 means fully lit by the scene light; 1 means ignore the
    // light and use the flat color (good for glowing things like searchlights).
    void drawBox(const glm::mat4& model, const glm::vec3& color,
                 float alpha = 1.0f, float emissive = 0.0f);
    void drawPyramid(const glm::mat4& model, const glm::vec3& color,
                     float alpha = 1.0f, float emissive = 0.0f);
    void drawCylinder(const glm::mat4& model, const glm::vec3& color,
                      float alpha = 1.0f, float emissive = 0.0f);
    void drawSphere(const glm::mat4& model, const glm::vec3& color,
                    float alpha = 1.0f, float emissive = 0.0f);

    // Convenience versions that build the matrix for you.
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

    // Translucent shapes should not write depth, or they hide things behind
    // them. Wrap a group of see-through draws in setDepthWrite(false/true).
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

//-----------------------------------------------------------------------------
// Builds a model matrix: move to `center`, rotate by `eulerDegrees` (applied
// Y then Z then X), then scale to `size`. Because the primitives are unit-sized
// and centered on the origin, `size` is the final world size of the shape.
//-----------------------------------------------------------------------------
glm::mat4 makeTransform(const glm::vec3& center,
                        const glm::vec3& size,
                        const glm::vec3& eulerDegrees = glm::vec3(0.0f));
