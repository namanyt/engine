#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
class Shader;

class ShaderLibrary final
{
  public:
    explicit ShaderLibrary(std::filesystem::path shaderDirectory);

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&&) = delete;
    ShaderLibrary& operator=(ShaderLibrary&&) = delete;

    const Shader& loadGraphicsProgram(const std::string& key, const std::string& vertexShaderName,
                                      const std::string& fragmentShaderName);
    std::filesystem::path shaderPath(const std::string& shaderName) const;
    const std::filesystem::path& shaderDirectory() const noexcept;

  private:
    std::filesystem::path m_shaderDirectory;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_programs;
};
} // namespace engine
