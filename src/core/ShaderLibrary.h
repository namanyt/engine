#pragma once

#include "core/Shader.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
class AssetManager;

class ShaderLibrary final
{
  public:
    ShaderLibrary(std::shared_ptr<AssetManager> assetManager,
                  std::filesystem::path shaderDirectory);

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&&) = delete;
    ShaderLibrary& operator=(ShaderLibrary&&) = delete;

    const Shader& loadGraphicsProgram(const std::string& key, const std::string& vertexShaderName,
                                      const std::string& fragmentShaderName);
    const Shader& loadGraphicsProgram(const std::string& key,
                                      const std::filesystem::path& vertexShaderPath,
                                      const std::filesystem::path& fragmentShaderPath);
    void unloadGraphicsProgram(const std::string& key) noexcept;
    bool hasGraphicsProgram(const std::string& key) const noexcept;
    std::filesystem::path shaderPath(const std::filesystem::path& shaderPath) const;
    const std::filesystem::path& shaderDirectory() const noexcept;

  private:
    std::shared_ptr<AssetManager> m_assetManager;
    std::filesystem::path m_shaderDirectory;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_programs;
};
} // namespace engine
