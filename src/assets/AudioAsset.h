#pragma once

#include "assets/Asset.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace engine
{
class AudioAsset final : public Asset
{
  public:
    static std::shared_ptr<AudioAsset> loadFromFile(const AssetMeta& meta);

    AudioAsset(AssetMeta meta, std::vector<std::uint8_t> encodedBytes, std::string formatName);
    ~AudioAsset() override;

    std::size_t sizeInBytes() const noexcept;
    const std::string& formatName() const noexcept;
    const std::vector<std::uint8_t>& encodedBytes() const noexcept;

  private:
    std::vector<std::uint8_t> m_encodedBytes;
    std::string m_formatName;
};
} // namespace engine
