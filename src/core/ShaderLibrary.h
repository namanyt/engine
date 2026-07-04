#pragma once

#include "core/Shader.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
class AssetManager;

/**
 * @brief Manages a cache of loaded shader programs keyed by string.
 *
 * Resolves shader file paths relative to a base directory and caches compiled
 * `Shader` objects. Programs are deduplicated by key; requesting the same key
 * twice returns the cached instance.
 *
 * @par Example
 * @code
 * auto& shader = library.loadGraphicsProgram("main", "vertex.glsl", "fragment.glsl");
 * shader.use();
 * @endcode
 *
 * @see Shader
 */
class ShaderLibrary final
{
  public:
    /// @brief Constructs a shader library with the given asset manager and base directory.
    /// @param assetManager Shared pointer to the asset manager.
    /// @param shaderDirectory Base path for resolving relative shader filenames.
    ShaderLibrary(std::shared_ptr<AssetManager> assetManager,
                  std::filesystem::path shaderDirectory);

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&&) = delete;
    ShaderLibrary& operator=(ShaderLibrary&&) = delete;

    /// @brief Loads a shader program by filename, resolving paths relative to the shader directory.
    /// @param key Unique cache key for this program.
    /// @param vertexShaderName Filename of the vertex shader (e.g. "vertex.glsl").
    /// @param fragmentShaderName Filename of the fragment shader (e.g. "fragment.glsl").
    /// @return Reference to the loaded (or cached) Shader.
    const Shader& loadGraphicsProgram(const std::string& key, const std::string& vertexShaderName,
                                      const std::string& fragmentShaderName);

    /// @brief Loads a shader program using absolute file paths.
    /// @param key Unique cache key for this program.
    /// @param vertexShaderPath Absolute path to the vertex shader file.
    /// @param fragmentShaderPath Absolute path to the fragment shader file.
    /// @return Reference to the loaded (or cached) Shader.
    const Shader& loadGraphicsProgram(const std::string& key,
                                      const std::filesystem::path& vertexShaderPath,
                                      const std::filesystem::path& fragmentShaderPath);

    /// @brief Removes a shader program from the cache by key.
    /// @param key Cache key of the program to unload.
    void unloadGraphicsProgram(const std::string& key) noexcept;

    /// @brief Checks whether a shader program with the given key is cached.
    /// @param key Cache key to look up.
    /// @return true if the program exists in the cache.
    bool hasGraphicsProgram(const std::string& key) const noexcept;

    /// @brief Resolves a relative shader path against the shader directory.
    /// @param shaderPath Relative or absolute path to resolve.
    /// @return Fully resolved filesystem path.
    std::filesystem::path shaderPath(const std::filesystem::path& shaderPath) const;

    /// @brief Returns the base shader directory path.
    /// @return Reference to the shader directory `std::filesystem::path`.
    const std::filesystem::path& shaderDirectory() const noexcept;

  private:
    std::shared_ptr<AssetManager> m_assetManager;
    std::filesystem::path m_shaderDirectory;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_programs;
};
} // namespace engine
