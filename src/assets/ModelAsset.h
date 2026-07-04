#pragma once

#include "assets/Asset.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace engine
{
/**
 * @brief Loaded 3D model asset with raw source bytes.
 *
 * Stores the raw file contents and format name (e.g. "obj", "gltf") for
 * later parsing by a model loader. Currently loads as binary data only;
 * vertex/face/normal parsing is not yet implemented.
 *
 * @see Asset
 * @see AssetManager
 */
class ModelAsset final : public Asset
{
  public:
    /// @brief Factory: reads a model file from disk into raw bytes.
    /// @param meta Asset metadata containing the source path.
    /// @return Shared pointer to the loaded ModelAsset, or nullptr on failure.
    static std::shared_ptr<ModelAsset> loadFromFile(const AssetMeta& meta);

    /// @brief Constructs a model asset with pre-loaded source data.
    /// @param meta Asset metadata.
    /// @param sourceBytes Raw file contents (e.g. OBJ or GLTF bytes).
    /// @param formatName Format identifier string (e.g. "obj", "gltf").
    ModelAsset(AssetMeta meta, std::vector<std::uint8_t> sourceBytes, std::string formatName);

    /// @brief Destroys the model asset.
    ~ModelAsset() override;

    /// @brief Returns the size of the raw model data in bytes.
    /// @return Byte count of the source payload.
    std::size_t sizeInBytes() const noexcept;

    /// @brief Returns the model format name.
    /// @return Reference to the format string (e.g. "obj", "gltf").
    const std::string& formatName() const noexcept;

    /// @brief Returns the raw model source bytes.
    /// @return Reference to the source byte buffer.
    const std::vector<std::uint8_t>& sourceBytes() const noexcept;

  private:
    std::vector<std::uint8_t> m_sourceBytes; ///< Raw model file data.
    std::string m_formatName;                ///< Format identifier (e.g. "obj").
};
} // namespace engine
