#include "assets/ModelAsset.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace engine
{
namespace
{
std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                   { return static_cast<char>(std::tolower(character)); });
    return value;
}
} // namespace

std::shared_ptr<ModelAsset> ModelAsset::loadFromFile(const AssetMeta& meta)
{
    std::ifstream file(meta.sourcePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open model asset: " + meta.sourcePath.string());
    }

    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    return std::make_shared<ModelAsset>(meta, std::move(bytes),
                                        toLower(meta.sourcePath.extension().string()).substr(1));
}

ModelAsset::ModelAsset(AssetMeta meta, std::vector<std::uint8_t> sourceBytes,
                       std::string formatName)
    : Asset(std::move(meta)), m_sourceBytes(std::move(sourceBytes)),
      m_formatName(std::move(formatName))
{
}

ModelAsset::~ModelAsset() = default;

std::size_t ModelAsset::sizeInBytes() const noexcept
{
    return m_sourceBytes.size();
}

const std::string& ModelAsset::formatName() const noexcept
{
    return m_formatName;
}

const std::vector<std::uint8_t>& ModelAsset::sourceBytes() const noexcept
{
    return m_sourceBytes;
}
} // namespace engine
