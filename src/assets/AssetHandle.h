#pragma once

#include "assets/AssetMeta.h"

#include <string>

namespace engine
{
class Asset;
class AudioAsset;
class ModelAsset;
class ShaderAsset;
class TextureAsset;

/// @brief Maps an asset class type to its corresponding `AssetType` enum value.
///
/// Specialize this template for custom asset types to integrate them with the
/// type-safe handle system.
template <typename TAsset> struct AssetTypeTraits;

template <> struct AssetTypeTraits<Asset>
{
    static constexpr AssetType type = AssetType::Unknown;
};

template <> struct AssetTypeTraits<TextureAsset>
{
    static constexpr AssetType type = AssetType::Texture;
};

template <> struct AssetTypeTraits<AudioAsset>
{
    static constexpr AssetType type = AssetType::Audio;
};

template <> struct AssetTypeTraits<ModelAsset>
{
    static constexpr AssetType type = AssetType::Model;
};

template <> struct AssetTypeTraits<ShaderAsset>
{
    static constexpr AssetType type = AssetType::Shader;
};

/**
 * @brief Lightweight, type-safe handle referencing an asset by UUID.
 *
 * `AssetHandle` is a value type (copyable, cheap to pass) that stores the
 * UUID and type of an asset. Use it to reference assets without holding
 * a pointer to the loaded data.
 *
 * @par Example
 * @code
 * AssetHandle<TextureAsset> handle = manager.findByPath<TextureAsset>("textures/bg.png.meta");
 * if (handle.isValid()) {
 *     auto tex = manager.load(handle);
 * }
 * @endcode
 *
 * @tparam TAsset The asset type (default: `Asset`).
 *
 * @see AssetManager
 * @see AssetMeta
 */
template <typename TAsset = Asset> class AssetHandle final
{
  public:
    /// @brief Constructs an invalid (empty) handle.
    AssetHandle() = default;

    /// @brief Constructs a handle with the given UUID and type.
    /// @param uuid Unique identifier string for the asset.
    /// @param type Classification of the asset.
    AssetHandle(std::string uuid, AssetType type) : m_uuid(std::move(uuid)), m_type(type) {}

    /// @brief Checks whether this handle references a valid asset.
    /// @return true if the UUID is non-empty.
    bool isValid() const noexcept
    {
        return !m_uuid.empty();
    }

    /// @brief Boolean conversion operator; returns true if the handle is valid.
    explicit operator bool() const noexcept
    {
        return isValid();
    }

    /// @brief Returns the UUID string for this asset.
    /// @return Reference to the internal UUID.
    const std::string& uuid() const noexcept
    {
        return m_uuid;
    }

    /// @brief Returns the asset type classification.
    /// @return The `AssetType` enum value.
    AssetType type() const noexcept
    {
        return m_type;
    }

    /// @brief Returns an untyped handle wrapping this reference.
    /// @return An `AssetHandle<Asset>` with the same UUID and type.
    AssetHandle<Asset> untyped() const
    {
        return AssetHandle<Asset>(m_uuid, m_type);
    }

    /// @brief Compares two handles for equality (same UUID and type).
    /// @param other Handle to compare against.
    /// @return true if both UUID and type match.
    bool operator==(const AssetHandle& other) const noexcept
    {
        return m_uuid == other.m_uuid && m_type == other.m_type;
    }

    /// @brief Compares two handles for inequality.
    /// @param other Handle to compare against.
    /// @return true if UUID or type differ.
    bool operator!=(const AssetHandle& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    std::string m_uuid;
    AssetType m_type = AssetType::Unknown;
};
} // namespace engine
