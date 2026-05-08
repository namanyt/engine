#include "assets/AudioAsset.h"

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

std::shared_ptr<AudioAsset> AudioAsset::loadFromFile(const AssetMeta& meta)
{
    std::ifstream file(meta.sourcePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open audio asset: " + meta.sourcePath.string());
    }

    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    return std::make_shared<AudioAsset>(meta, std::move(bytes),
                                        toLower(meta.sourcePath.extension().string()).substr(1));
}

AudioAsset::AudioAsset(AssetMeta meta, std::vector<std::uint8_t> encodedBytes,
                       std::string formatName)
    : Asset(std::move(meta)), m_encodedBytes(std::move(encodedBytes)),
      m_formatName(std::move(formatName))
{
}

AudioAsset::~AudioAsset() = default;

std::size_t AudioAsset::sizeInBytes() const noexcept
{
    return m_encodedBytes.size();
}

const std::string& AudioAsset::formatName() const noexcept
{
    return m_formatName;
}

const std::vector<std::uint8_t>& AudioAsset::encodedBytes() const noexcept
{
    return m_encodedBytes;
}
} // namespace engine
