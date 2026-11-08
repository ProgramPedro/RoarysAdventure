#pragma once

// A wrapper around one VAO/VBO/EBO triple, plus builders for the handful of
// primitives the whole game is drawn from: box, pyramid, cylinder, sphere and a
// fullscreen quad. A mesh owns GPU resources, so it can be moved but not copied.

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

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices);

    void draw() const;
    void destroy();

    void buildBox();

    void buildPyramid();

    void buildCylinder(int segments = 24);

    void buildSphere(int segments = 20, int rings = 12);

    void buildFullscreenQuad();

private:
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
    int indexCount_ = 0;
};
