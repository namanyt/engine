#pragma once

#include "assets/AssetMeta.h"

namespace engine
{
class Asset
{
  public:
    explicit Asset(AssetMeta meta);
    virtual ~Asset();

    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;
    Asset(Asset&&) = delete;
    Asset& operator=(Asset&&) = delete;

    const std::string& uuid() const noexcept;
    AssetType type() const noexcept;
    const std::filesystem::path& sourcePath() const noexcept;
    const AssetMeta& meta() const noexcept;

  protected:
    AssetMeta m_meta;
};
} // namespace engine
