#include "core/Shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace {

// Compiles one shader stage. Returns 0 on failure after printing the log.
unsigned int compileStage(unsigned int type, const char* source) {
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        char log[1024]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        const char* stageName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        std::cerr << "[Shader] " << stageName << " compile failed:\n" << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

Shader::~Shader() {
    destroy();
}

bool Shader::build(const char* vertexSource, const char* fragmentSource) {
    destroy();

    const unsigned int vs = compileStage(GL_VERTEX_SHADER, vertexSource);
    if (vs == 0) return false;

    const unsigned int fs = compileStage(GL_FRAGMENT_SHADER, fragmentSource);
    if (fs == 0) {
        glDeleteShader(vs);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    // The stages are baked into the program now; the objects can go away.
    glDeleteShader(vs);
    glDeleteShader(fs);

    int success = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        char log[1024]{};
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::cerr << "[Shader] link failed:\n" << log << '\n';
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    return true;
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::destroy() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    uniformCache_.clear();
}

int Shader::location(const char* name) {
    // Look in the cache first; only ask the driver once per uniform name.
    const auto it = uniformCache_.find(name);
    if (it != uniformCache_.end()) {
        return it->second;
    }
    const int loc = glGetUniformLocation(program_, name);
    uniformCache_.emplace(name, loc);
    return loc;
}

void Shader::setMat4(const char* name, const glm::mat4& value) {
    glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat3(const char* name, const glm::mat3& value) {
    glUniformMatrix3fv(location(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) {
    glUniform3f(location(name), value.x, value.y, value.z);
}

void Shader::setVec2(const char* name, const glm::vec2& value) {
    glUniform2f(location(name), value.x, value.y);
}

void Shader::setFloat(const char* name, float value) {
    glUniform1f(location(name), value);
}

void Shader::setInt(const char* name, int value) {
    glUniform1i(location(name), value);
}
