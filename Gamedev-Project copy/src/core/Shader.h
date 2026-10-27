#pragma once
//=============================================================================
// Shader - compiles a vertex + fragment shader pair into a GL program and
// gives us small helpers for setting uniforms.
//
// Uniform locations are cached in a map the first time they are requested, so
// we are not calling glGetUniformLocation hundreds of times per frame.
//=============================================================================

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader() = default;
    ~Shader();

    // Shaders own a GL resource, so copying them would double-delete it.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Compiles and links the two sources. Returns false and prints the GL
    // error log to stderr if anything fails.
    bool build(const char* vertexSource, const char* fragmentSource);

    void use() const;
    void destroy();

    bool isValid() const { return program_ != 0; }

    // --- uniform setters -----------------------------------------------------
    void setMat4(const char* name, const glm::mat4& value);
    void setMat3(const char* name, const glm::mat3& value);
    void setVec3(const char* name, const glm::vec3& value);
    void setVec2(const char* name, const glm::vec2& value);
    void setFloat(const char* name, float value);
    void setInt(const char* name, int value);

private:
    int location(const char* name);

    unsigned int program_ = 0;
    std::unordered_map<std::string, int> uniformCache_;
};
