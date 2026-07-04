#pragma once

#include "assets/Asset.h"

#include <memory>
#include <string>

namespace engine
{
/**
 * @brief Loaded shader source asset.
 *
 * Stores the GLSL source text and inferred shader stage (vertex or fragment)
 * for a shader file. Used by `ShaderLibrary` to compile programs.
 *
 * @see Asset
 * @see Shader
 * @see ShaderLibrary
 */
class ShaderAsset final : public Asset
{
  public:
    /// @brief Factory: reads a shader source file from disk.
    /// @param meta Asset metadata containing the source path.
    /// @return Shared pointer to the loaded ShaderAsset, or nullptr on failure.
    static std::shared_ptr<ShaderAsset> loadFromFile(const AssetMeta& meta);

    /// @brief Constructs a shader asset with pre-loaded source text.
    /// @param meta Asset metadata.
    /// @param source GLSL source code string.
    /// @param stage Inferred shader stage (vertex or fragment).
    ShaderAsset(AssetMeta meta, std::string source, ShaderStage stage);

    /// @brief Destroys the shader asset.
    ~ShaderAsset() override;

    /// @brief Returns the GLSL source code text.
    /// @return Reference to the source string.
    const std::string& source() const noexcept;

    /// @brief Returns the inferred shader stage.
    /// @return ShaderStage enum value (Vertex, Fragment, or Unknown).
    ShaderStage stage() const noexcept;

  private:
    std::string m_source;                       ///< GLSL source code text.
    ShaderStage m_stage = ShaderStage::Unknown; ///< Inferred shader stage.
};
} // namespace engine
