#pragma once

#include "assets/Asset.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace engine
{
class ModelAsset final : public Asset
{
  public:
    static std::shared_ptr<ModelAsset> loadFromFile(const AssetMeta& meta);

    ModelAsset(AssetMeta meta, std::vector<std::uint8_t> sourceBytes, std::string formatName);
    ~ModelAsset() override;

    std::size_t sizeInBytes() const noexcept;
    const std::string& formatName() const noexcept;
    const std::vector<std::uint8_t>& sourceBytes() const noexcept;

  private:
    std::vector<std::uint8_t> m_sourceBytes;
    std::string m_formatName;
};
} // namespace engine
