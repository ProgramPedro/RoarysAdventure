#pragma once

// Compiles a vertex and fragment shader pair into a GL program and provides small
// helpers for setting uniforms. Uniform locations are cached the first time they
// are asked for, so the game is not calling glGetUniformLocation hundreds of
// times per frame. A shader owns a GL resource, so it cannot be copied.

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

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool build(const char* vertexSource, const char* fragmentSource);

    void use() const;
    void destroy();

    bool isValid() const { return program_ != 0; }

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
