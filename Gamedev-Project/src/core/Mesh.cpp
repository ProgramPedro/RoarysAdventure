// Uploads vertex and index data to the GPU, releases it on destruction, and
// generates each primitive shape. The cylinder and sphere use smooth radial
// normals, which is what makes the tower read as round rather than faceted. Every
// builder fills the same Vertex list, so a real model loader could drop in here.

#include "core/Mesh.h"

#include <glad/glad.h>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

Mesh::~Mesh() {
    destroy();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_),
      indexCount_(other.indexCount_) {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
    other.indexCount_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        destroy();
        vao_ = other.vao_;
        vbo_ = other.vbo_;
        ebo_ = other.ebo_;
        indexCount_ = other.indexCount_;
        other.vao_ = 0;
        other.vbo_ = 0;
        other.ebo_ = 0;
        other.indexCount_ = 0;
    }
    return *this;
}

void Mesh::upload(const std::vector<Vertex>& vertices,
                  const std::vector<unsigned int>& indices) {
    destroy();

    indexCount_ = static_cast<int>(indices.size());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (indexCount_ == 0) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::destroy() {
    if (ebo_ != 0) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    indexCount_ = 0;
}

void Mesh::buildBox() {
    const glm::vec3 corners[6][4] = {
        {{-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},

        {{ 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}},

        {{ 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}},

        {{-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}},

        {{-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}},

        {{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}},
    };
    const glm::vec3 normals[6] = {
        { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f,-1.0f},
        { 1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        { 0.0f, 1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f},
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(24);
    indices.reserve(36);

    for (int face = 0; face < 6; ++face) {
        const unsigned int base = static_cast<unsigned int>(vertices.size());
        for (int i = 0; i < 4; ++i) {
            vertices.push_back(Vertex{corners[face][i], normals[face]});
        }

        indices.insert(indices.end(), {base + 0, base + 1, base + 2,
                                       base + 2, base + 3, base + 0});
    }

    upload(vertices, indices);
}

void Mesh::buildPyramid() {
    const glm::vec3 apex{0.0f, 0.5f, 0.0f};
    const glm::vec3 b0{-0.5f, -0.5f,  0.5f};
    const glm::vec3 b1{ 0.5f, -0.5f,  0.5f};
    const glm::vec3 b2{ 0.5f, -0.5f, -0.5f};
    const glm::vec3 b3{-0.5f, -0.5f, -0.5f};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addTriangle = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        const glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
        const unsigned int base = static_cast<unsigned int>(vertices.size());
        vertices.push_back(Vertex{a, n});
        vertices.push_back(Vertex{b, n});
        vertices.push_back(Vertex{c, n});
        indices.insert(indices.end(), {base + 0, base + 1, base + 2});
    };

    addTriangle(b0, b1, apex);
    addTriangle(b1, b2, apex);
    addTriangle(b2, b3, apex);
    addTriangle(b3, b0, apex);

    addTriangle(b3, b2, b1);
    addTriangle(b1, b0, b3);

    upload(vertices, indices);
}

void Mesh::buildCylinder(int segments) {
    segments = std::max(segments, 3);

    constexpr float kRadius = 0.5f;
    constexpr float kHalfHeight = 0.5f;
    constexpr float kTwoPi = 6.283185307f;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i < segments; ++i) {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
        const float sinA = std::sin(angle);
        const float cosA = std::cos(angle);
        const glm::vec3 normal(sinA, 0.0f, cosA);

        vertices.push_back(Vertex{{kRadius * sinA, -kHalfHeight, kRadius * cosA}, normal});
        vertices.push_back(Vertex{{kRadius * sinA,  kHalfHeight, kRadius * cosA}, normal});
    }

    for (int i = 0; i < segments; ++i) {
        const unsigned int bottom0 = static_cast<unsigned int>(i * 2);
        const unsigned int top0 = bottom0 + 1;
        const unsigned int bottom1 = static_cast<unsigned int>(((i + 1) % segments) * 2);
        const unsigned int top1 = bottom1 + 1;

        indices.insert(indices.end(), {bottom0, bottom1, top1,
                                       top1, top0, bottom0});
    }

    auto addCap = [&](float y, const glm::vec3& normal, bool flipWinding) {
        const unsigned int center = static_cast<unsigned int>(vertices.size());
        vertices.push_back(Vertex{{0.0f, y, 0.0f}, normal});

        for (int i = 0; i < segments; ++i) {
            const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
            vertices.push_back(Vertex{
                {kRadius * std::sin(angle), y, kRadius * std::cos(angle)}, normal});
        }

        for (int i = 0; i < segments; ++i) {
            const unsigned int a = center + 1 + static_cast<unsigned int>(i);
            const unsigned int b = center + 1 + static_cast<unsigned int>((i + 1) % segments);
            if (flipWinding) {
                indices.insert(indices.end(), {center, b, a});
            } else {
                indices.insert(indices.end(), {center, a, b});
            }
        }
    };

    addCap( kHalfHeight, glm::vec3(0.0f,  1.0f, 0.0f), false);
    addCap(-kHalfHeight, glm::vec3(0.0f, -1.0f, 0.0f), true);

    upload(vertices, indices);
}

void Mesh::buildSphere(int segments, int rings) {
    segments = std::max(segments, 3);
    rings = std::max(rings, 2);

    constexpr float kPi = 3.14159265f;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int ring = 0; ring <= rings; ++ring) {
        const float phi = kPi * static_cast<float>(ring) / static_cast<float>(rings);
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (int segment = 0; segment <= segments; ++segment) {
            const float theta = 2.0f * kPi * static_cast<float>(segment)
                              / static_cast<float>(segments);
            const glm::vec3 direction(sinPhi * std::sin(theta), cosPhi,
                                      sinPhi * std::cos(theta));
            vertices.push_back(Vertex{direction * 0.5f, direction});
        }
    }

    const int stride = segments + 1;
    for (int ring = 0; ring < rings; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            const unsigned int topLeft =
                static_cast<unsigned int>(ring * stride + segment);
            const unsigned int bottomLeft = topLeft + static_cast<unsigned int>(stride);

            indices.insert(indices.end(), {topLeft, bottomLeft, topLeft + 1,
                                           topLeft + 1, bottomLeft, bottomLeft + 1});
        }
    }

    upload(vertices, indices);
}

void Mesh::buildFullscreenQuad() {
    const std::vector<Vertex> vertices = {
        Vertex{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        Vertex{{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        Vertex{{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        Vertex{{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };
    const std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};
    upload(vertices, indices);
}
