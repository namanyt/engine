#pragma once

#include <filesystem>
#include <string>

namespace engine
{
struct Mat4;

class Shader final
{
  public:
    Shader(const std::filesystem::path& vertexShaderPath,
           const std::filesystem::path& fragmentShaderPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void use() const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setMat4(const std::string& name, const Mat4& value) const;
    unsigned int programId() const noexcept;

  private:
    static std::string readTextFile(const std::filesystem::path& path);
    static unsigned int compileStage(unsigned int stage, const std::string& source,
                                     const std::filesystem::path& path);
    static void validateProgramLink(unsigned int programId);

    int uniformLocation(const std::string& name) const;

    unsigned int m_programId = 0;
};
} // namespace engine
