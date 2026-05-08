#pragma once

#include "assets/Asset.h"

#include <memory>

namespace engine
{
class TextureAsset final : public Asset
{
  public:
    static std::shared_ptr<TextureAsset> loadFromFile(const AssetMeta& meta);

    TextureAsset(AssetMeta meta, unsigned int textureId, int width, int height, int channelCount,
                 bool hdr);
    ~TextureAsset() override;

    unsigned int textureId() const noexcept;
    int width() const noexcept;
    int height() const noexcept;
    int channelCount() const noexcept;
    bool isHdr() const noexcept;

  private:
    unsigned int m_textureId = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channelCount = 0;
    bool m_isHdr = false;
};
} // namespace engine
