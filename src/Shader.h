#pragma once

#include <filesystem>
#include <string>

namespace engine
{
class Shader final
{
public:
    Shader(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void use() const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, float x, float y) const;

private:
    static std::string readTextFile(const std::filesystem::path& path);
    static unsigned int compileStage(unsigned int stage, const std::string& source, const std::filesystem::path& path);
    static void validateProgramLink(unsigned int programId);

    unsigned int m_programId = 0;
};
} // namespace engine
