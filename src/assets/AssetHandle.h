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

template <typename TAsset = Asset> class AssetHandle final
{
  public:
    AssetHandle() = default;

    AssetHandle(std::string uuid, AssetType type) : m_uuid(std::move(uuid)), m_type(type) {}

    bool isValid() const noexcept
    {
        return !m_uuid.empty();
    }

    explicit operator bool() const noexcept
    {
        return isValid();
    }

    const std::string& uuid() const noexcept
    {
        return m_uuid;
    }

    AssetType type() const noexcept
    {
        return m_type;
    }

    AssetHandle<Asset> untyped() const
    {
        return AssetHandle<Asset>(m_uuid, m_type);
    }

    bool operator==(const AssetHandle& other) const noexcept
    {
        return m_uuid == other.m_uuid && m_type == other.m_type;
    }

    bool operator!=(const AssetHandle& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    std::string m_uuid;
    AssetType m_type = AssetType::Unknown;
};
} // namespace engine
