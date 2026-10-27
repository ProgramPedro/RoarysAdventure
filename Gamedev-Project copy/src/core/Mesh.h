#pragma once
//=============================================================================
// Mesh - a thin wrapper around one VAO/VBO/EBO triple plus factory functions
// for the handful of primitive shapes the whole game is built from.
//
// Every placeholder object in the game is a box or a pyramid. When real models
// arrive, add a loader that fills the same Vertex list and nothing else has to
// change.
//=============================================================================

#include <glm/vec3.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    // Meshes own GL resources, so they must not be copied. Moving is allowed
    // so they can live in containers.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Uploads geometry to the GPU.
    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices);

    void draw() const;
    void destroy();

    // --- primitive builders --------------------------------------------------
    // A unit cube centered on the origin, spanning -0.5..0.5 on every axis.
    // Scale it with a model matrix to get any box you need.
    void buildBox();

    // A square-based pyramid: base at y = -0.5 spanning -0.5..0.5 in X and Z,
    // apex at (0, +0.5, 0). Used for ears, horns, beaks and roof spikes.
    void buildPyramid();

    // A cylinder standing on the Y axis: radius 0.5, height 1, centered on the
    // origin, so scaling by (diameter, height, diameter) gives any size you
    // want. The curved side uses smooth radial normals, which is what makes
    // the tower read as round rather than faceted.
    void buildCylinder(int segments = 24);

    // A sphere of radius 0.5 centered on the origin, with smooth normals.
    // Squash or stretch it with a model matrix to get the rounded, plush
    // shapes the mascot costumes are made of.
    void buildSphere(int segments = 20, int rings = 12);

    // A screen-filling quad in normalized device coordinates, for the
    // background gradient. Has no meaningful normals.
    void buildFullscreenQuad();

private:
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
    int indexCount_ = 0;
};
