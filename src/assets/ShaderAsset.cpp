#include "assets/ShaderAsset.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace engine
{
std::shared_ptr<ShaderAsset> ShaderAsset::loadFromFile(const AssetMeta& meta)
{
    std::ifstream file(meta.sourcePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader asset: " + meta.sourcePath.string());
    }

    const std::string source((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    return std::make_shared<ShaderAsset>(meta, source, inferShaderStageFromPath(meta.sourcePath));
}

ShaderAsset::ShaderAsset(AssetMeta meta, std::string source, ShaderStage stage)
    : Asset(std::move(meta)), m_source(std::move(source)), m_stage(stage)
{
}

ShaderAsset::~ShaderAsset() = default;

const std::string& ShaderAsset::source() const noexcept
{
    return m_source;
}

ShaderStage ShaderAsset::stage() const noexcept
{
    return m_stage;
}
} // namespace engine
