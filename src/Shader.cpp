#include "Shader.h"

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace engine
{
Shader::Shader(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath)
{
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

void Shader::setFloat(const std::string& name, float value) const
{
    const int location = glGetUniformLocation(m_programId, name.c_str());
    if (location != -1)
    {
        glUniform1f(location, value);
    }
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
    const int location = glGetUniformLocation(m_programId, name.c_str());
    if (location != -1)
    {
        glUniform2f(location, x, y);
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
} // namespace engine
