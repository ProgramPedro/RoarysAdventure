#include "core/Renderer.h"

#include "core/Config.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>

namespace {

//-----------------------------------------------------------------------------
// The one shader every solid object in the game uses. A single directional
// light with an ambient floor: enough shading to make boxes read as 3D without
// any lighting setup in the scenes.
//-----------------------------------------------------------------------------
constexpr const char* kLitVertexShader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;

void main() {
    vNormal = uNormalMatrix * aNormal;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)GLSL";

constexpr const char* kLitFragmentShader = R"GLSL(
#version 330 core
in vec3 vNormal;
out vec4 FragColor;

uniform vec3  uColor;
uniform vec3  uLightDirection;  // direction the light travels, normalized
uniform float uAlpha;
uniform float uEmissive;        // 0 = fully lit, 1 = ignore the light

void main() {
    vec3 normal = normalize(vNormal);

    // Standard Lambert term: how directly this face points at the light.
    float diffuse = max(dot(normal, -uLightDirection), 0.0);

    const float ambient = 0.42;
    vec3 shaded = uColor * (ambient + 0.70 * diffuse);

    // Blend toward the raw color for glowing objects.
    vec3 finalColor = mix(shaded, uColor, clamp(uEmissive, 0.0, 1.0));

    FragColor = vec4(finalColor, uAlpha);
}
)GLSL";

//-----------------------------------------------------------------------------
// Background: a full-screen quad already in clip space. The fragment shader
// mixes two colors vertically and adds a soft radial glow, which is what gives
// the menu its FIU blue-to-gold look.
//-----------------------------------------------------------------------------
constexpr const char* kBackgroundVertexShader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPosition;

out vec2 vNdc;

void main() {
    vNdc = aPosition.xy;
    gl_Position = vec4(aPosition.xy, 0.0, 1.0);
}
)GLSL";

constexpr const char* kBackgroundFragmentShader = R"GLSL(
#version 330 core
in vec2 vNdc;
out vec4 FragColor;

uniform vec3  uTopColor;
uniform vec3  uBottomColor;
uniform vec3  uGlowColor;
uniform vec2  uGlowCenter;
uniform float uGlowRadius;
uniform float uGlowStrength;
uniform float uAspect;

void main() {
    // Vertical gradient. vNdc.y runs -1 (bottom) to +1 (top).
    float t = vNdc.y * 0.5 + 0.5;
    vec3 color = mix(uBottomColor, uTopColor, t);

    // Radial glow. Correct x by the aspect ratio so the glow stays round
    // instead of stretching when the window is resized.
    vec2 offset = vNdc - uGlowCenter;
    offset.x *= uAspect;
    float distance = length(offset);

    // Smooth falloff from the center out to uGlowRadius.
    float glow = 1.0 - smoothstep(0.0, uGlowRadius, distance);
    color += uGlowColor * glow * glow * uGlowStrength;

    FragColor = vec4(color, 1.0);
}
)GLSL";

} // namespace

glm::mat4 makeTransform(const glm::vec3& center,
                        const glm::vec3& size,
                        const glm::vec3& eulerDegrees) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, center);

    // Rotations are applied Y, then Z, then X. Y is the useful one for turning
    // objects to face the camera; Z tilts things like horns and searchlights.
    if (eulerDegrees.y != 0.0f) {
        model = glm::rotate(model, glm::radians(eulerDegrees.y), glm::vec3(0, 1, 0));
    }
    if (eulerDegrees.z != 0.0f) {
        model = glm::rotate(model, glm::radians(eulerDegrees.z), glm::vec3(0, 0, 1));
    }
    if (eulerDegrees.x != 0.0f) {
        model = glm::rotate(model, glm::radians(eulerDegrees.x), glm::vec3(1, 0, 0));
    }

    model = glm::scale(model, size);
    return model;
}

bool Renderer::init() {
    if (!litShader_.build(kLitVertexShader, kLitFragmentShader)) {
        return false;
    }
    if (!backgroundShader_.build(kBackgroundVertexShader, kBackgroundFragmentShader)) {
        return false;
    }
    if (!text_.init()) {
        return false;
    }

    boxMesh_.buildBox();
    pyramidMesh_.buildPyramid();
    cylinderMesh_.buildCylinder(28);
    sphereMesh_.buildSphere(20, 12);
    fullscreenQuad_.buildFullscreenQuad();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Face culling is left OFF on purpose. Every placeholder shape is a closed
    // solid, so the depth test already hides the back faces, and leaving
    // culling off means a shape with reversed winding still looks correct.
    glDisable(GL_CULL_FACE);

    return true;
}

void Renderer::cleanup() {
    text_.cleanup();
    fullscreenQuad_.destroy();
    sphereMesh_.destroy();
    cylinderMesh_.destroy();
    pyramidMesh_.destroy();
    boxMesh_.destroy();
    backgroundShader_.destroy();
    litShader_.destroy();
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight) {
    glViewport(0, 0, std::max(framebufferWidth, 1), std::max(framebufferHeight, 1));

    // Only depth needs clearing: drawBackground() covers every pixel.
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawBackground(const glm::vec3& topColor,
                              const glm::vec3& bottomColor,
                              const glm::vec3& glowColor,
                              const glm::vec2& glowCenterNdc,
                              float glowRadius,
                              float glowStrength) {
    // The background sits behind everything, so it neither tests nor writes
    // depth. It is the first thing drawn each frame.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    int viewport[4]{};
    glGetIntegerv(GL_VIEWPORT, viewport);
    const float aspect = (viewport[3] > 0)
        ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3])
        : 1.0f;

    backgroundShader_.use();
    backgroundShader_.setVec3("uTopColor", topColor);
    backgroundShader_.setVec3("uBottomColor", bottomColor);
    backgroundShader_.setVec3("uGlowColor", glowColor);
    backgroundShader_.setVec2("uGlowCenter", glowCenterNdc);
    backgroundShader_.setFloat("uGlowRadius", glowRadius);
    backgroundShader_.setFloat("uGlowStrength", glowStrength);
    backgroundShader_.setFloat("uAspect", aspect);

    fullscreenQuad_.draw();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::setCamera(const glm::mat4& view, const glm::mat4& projection) {
    view_ = view;
    projection_ = projection;
    viewProjection_ = projection * view;

    litShader_.use();
    litShader_.setMat4("uView", view_);
    litShader_.setMat4("uProjection", projection_);
    litShader_.setVec3("uLightDirection", glm::normalize(cfg::kLightDirection));
}

void Renderer::applyMaterial(const glm::mat4& model, const glm::vec3& color,
                             float alpha, float emissive) {
    litShader_.use();
    litShader_.setMat4("uModel", model);

    // The normal matrix is the inverse-transpose of the model matrix's upper
    // 3x3. Without it, non-uniform scaling (which we use constantly, since
    // every shape is a stretched unit cube) would skew the normals and the
    // shading would come out wrong.
    litShader_.setMat3("uNormalMatrix",
                       glm::transpose(glm::inverse(glm::mat3(model))));

    litShader_.setVec3("uColor", color);
    litShader_.setFloat("uAlpha", alpha);
    litShader_.setFloat("uEmissive", emissive);
}

void Renderer::drawBox(const glm::mat4& model, const glm::vec3& color,
                       float alpha, float emissive) {
    applyMaterial(model, color, alpha, emissive);
    boxMesh_.draw();
}

void Renderer::drawPyramid(const glm::mat4& model, const glm::vec3& color,
                           float alpha, float emissive) {
    applyMaterial(model, color, alpha, emissive);
    pyramidMesh_.draw();
}

void Renderer::drawCylinder(const glm::mat4& model, const glm::vec3& color,
                            float alpha, float emissive) {
    applyMaterial(model, color, alpha, emissive);
    cylinderMesh_.draw();
}

void Renderer::drawSphere(const glm::mat4& model, const glm::vec3& color,
                          float alpha, float emissive) {
    applyMaterial(model, color, alpha, emissive);
    sphereMesh_.draw();
}

void Renderer::drawBoxAt(const glm::vec3& center, const glm::vec3& size,
                         const glm::vec3& color, float alpha, float emissive) {
    drawBox(makeTransform(center, size), color, alpha, emissive);
}

void Renderer::drawPyramidAt(const glm::vec3& center, const glm::vec3& size,
                             const glm::vec3& color, float alpha, float emissive) {
    drawPyramid(makeTransform(center, size), color, alpha, emissive);
}

void Renderer::drawCylinderAt(const glm::vec3& center, const glm::vec3& size,
                              const glm::vec3& color, float alpha, float emissive) {
    drawCylinder(makeTransform(center, size), color, alpha, emissive);
}

void Renderer::drawSphereAt(const glm::vec3& center, const glm::vec3& size,
                            const glm::vec3& color, float alpha, float emissive) {
    drawSphere(makeTransform(center, size), color, alpha, emissive);
}

void Renderer::setDepthWrite(bool enabled) {
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}
