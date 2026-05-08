#pragma once

#include "assets/Asset.h"

#include <memory>
#include <string>

namespace engine
{
class ShaderAsset final : public Asset
{
  public:
    static std::shared_ptr<ShaderAsset> loadFromFile(const AssetMeta& meta);

    ShaderAsset(AssetMeta meta, std::string source, ShaderStage stage);
    ~ShaderAsset() override;

    const std::string& source() const noexcept;
    ShaderStage stage() const noexcept;

  private:
    std::string m_source;
    ShaderStage m_stage = ShaderStage::Unknown;
};
} // namespace engine
