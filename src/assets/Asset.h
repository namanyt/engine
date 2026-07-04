#pragma once

#include "assets/AssetMeta.h"

namespace engine
{
/**
 * @brief Base class for all engine asset types.
 *
 * `Asset` owns the metadata (UUID, type, source path) for a loaded resource.
 * Derived classes (`TextureAsset`, `AudioAsset`, etc.) add type-specific
 * data such as GPU texture IDs or decoded audio buffers.
 *
 * @see TextureAsset
 * @see AudioAsset
 * @see ModelAsset
 * @see ShaderAsset
 */
class Asset
{
  public:
    /// @brief Constructs an asset with the given metadata.
    /// @param meta Asset metadata (UUID, type, source path).
    explicit Asset(AssetMeta meta);

    /// @brief Virtual destructor for polymorphic deletion via shared_ptr.
    virtual ~Asset();

    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;
    Asset(Asset&&) = delete;
    Asset& operator=(Asset&&) = delete;

    /// @brief Returns the asset's UUID string.
    /// @return Reference to the internal UUID.
    const std::string& uuid() const noexcept;

    /// @brief Returns the asset's type classification.
    /// @return The `AssetType` enum value.
    AssetType type() const noexcept;

    /// @brief Returns the source file path for this asset.
    /// @return Reference to the filesystem path.
    const std::filesystem::path& sourcePath() const noexcept;

    /// @brief Returns the full metadata record for this asset.
    /// @return Const reference to the `AssetMeta`.
    const AssetMeta& meta() const noexcept;

  protected:
    AssetMeta m_meta; ///< Persistent metadata for this asset.
};
} // namespace engine
