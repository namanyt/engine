#include "assets/Asset.h"

namespace engine
{
Asset::Asset(AssetMeta meta) : m_meta(std::move(meta)) {}

Asset::~Asset() = default;

const std::string& Asset::uuid() const noexcept
{
    return m_meta.uuid;
}

AssetType Asset::type() const noexcept
{
    return m_meta.type;
}

const std::filesystem::path& Asset::sourcePath() const noexcept
{
    return m_meta.sourcePath;
}

const AssetMeta& Asset::meta() const noexcept
{
    return m_meta;
}
} // namespace engine
