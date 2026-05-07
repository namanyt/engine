#include "core/Shader.h"

#include "core/Log.h"
#include "math/Transform.h"

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace engine
{
Shader::Shader(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath)
{
    {
        std::ostringstream stream;
        stream << "Loading vertex shader: " << vertexShaderPath.string();
        Log::info("Shader", stream.str());
    }

    {
        std::ostringstream stream;
        stream << "Loading fragment shader: " << fragmentShaderPath.string();
        Log::info("Shader", stream.str());
    }

    const std::string vertexSource = readTextFile(vertexShaderPath);
    const std::string fragmentSource = readTextFile(fragmentShaderPath);

    const unsigned int vertexShader = compileStage(GL_VERTEX_SHADER, vertexSource, vertexShaderPath);
    const unsigned int fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentShaderPath);

    m_programId = glCreateProgram();
    if (m_programId == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error("Failed to create an OpenGL shader program.");
    }

    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    try
    {
        validateProgramLink(m_programId);
    }
    catch (...)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_programId);
        m_programId = 0;
        throw;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    {
        std::ostringstream stream;
        stream << "Shader program linked successfully. Program ID: " << m_programId;
        Log::info("Shader", stream.str());
    }
}

Shader::~Shader()
{
    if (m_programId != 0)
    {
        glDeleteProgram(m_programId);
    }
}

void Shader::use() const
{
    glUseProgram(m_programId);
}

void Shader::setInt(const std::string& name, int value) const
{
    const int location = uniformLocation(name);
    if (location != -1)
    {
        glUniform1i(location, value);
    }
}

void Shader::setFloat(const std::string& name, float value) const
{
    const int location = uniformLocation(name);
    if (location != -1)
    {
        glUniform1f(location, value);
    }
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
    const int location = uniformLocation(name);
    if (location != -1)
    {
        glUniform2f(location, x, y);
    }
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    const int location = uniformLocation(name);
    if (location != -1)
    {
        glUniform3f(location, x, y, z);
    }
}

void Shader::setMat4(const std::string& name, const Mat4& value) const
{
    const int location = uniformLocation(name);
    if (location != -1)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, value.data());
    }
}

std::string Shader::readTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + path.string());
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

unsigned int Shader::compileStage(unsigned int stage, const std::string& source, const std::filesystem::path& path)
{
    const unsigned int shaderId = glCreateShader(stage);
    if (shaderId == 0)
    {
        throw std::runtime_error("Failed to create shader stage for file: " + path.string());
    }

    const char* sourcePointer = source.c_str();
    glShaderSource(shaderId, 1, &sourcePointer, nullptr);
    glCompileShader(shaderId);

    int compiled = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE)
    {
        int logLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

        std::string infoLog(static_cast<std::size_t>(logLength > 0 ? logLength : 1), '\0');
        glGetShaderInfoLog(shaderId, logLength, nullptr, infoLog.data());

        glDeleteShader(shaderId);
        throw std::runtime_error("Shader compilation failed for '" + path.string() + "':\n" + infoLog);
    }

    {
        const char* stageName = stage == GL_VERTEX_SHADER ? "vertex" : "fragment";
        std::ostringstream stream;
        stream << "Compiled " << stageName << " shader: " << path.string();
        Log::info("Shader", stream.str());
    }

    return shaderId;
}

void Shader::validateProgramLink(unsigned int programId)
{
    int linked = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE)
    {
        int logLength = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

        std::string infoLog(static_cast<std::size_t>(logLength > 0 ? logLength : 1), '\0');
        glGetProgramInfoLog(programId, logLength, nullptr, infoLog.data());

        throw std::runtime_error("Shader program linking failed:\n" + infoLog);
    }
}

int Shader::uniformLocation(const std::string& name) const
{
    return glGetUniformLocation(m_programId, name.c_str());
}
} // namespace engine
