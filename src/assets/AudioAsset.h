#pragma once

#include "assets/Asset.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace engine
{
/**
 * @brief Loaded audio asset with encoded byte data.
 *
 * Stores the raw encoded audio bytes and format name (e.g. "mp3", "wav",
 * "ogg") for later decoding by the audio subsystem.
 *
 * @see Asset
 * @see AssetManager
 */
class AudioAsset final : public Asset
{
  public:
    /// @brief Factory: loads an audio file from disk into raw bytes.
    /// @param meta Asset metadata containing the source path.
    /// @return Shared pointer to the loaded AudioAsset, or nullptr on failure.
    static std::shared_ptr<AudioAsset> loadFromFile(const AssetMeta& meta);

    /// @brief Constructs an audio asset with pre-loaded encoded data.
    /// @param meta Asset metadata.
    /// @param encodedBytes Raw encoded audio data (e.g. MP3, WAV bytes).
    /// @param formatName Format identifier string (e.g. "mp3", "wav").
    AudioAsset(AssetMeta meta, std::vector<std::uint8_t> encodedBytes, std::string formatName);

    /// @brief Destroys the audio asset.
    ~AudioAsset() override;

    /// @brief Returns the size of the encoded audio data in bytes.
    /// @return Byte count of the encoded payload.
    std::size_t sizeInBytes() const noexcept;

    /// @brief Returns the audio format name.
    /// @return Reference to the format string (e.g. "mp3", "wav").
    const std::string& formatName() const noexcept;

    /// @brief Returns the raw encoded audio bytes.
    /// @return Reference to the encoded byte buffer.
    const std::vector<std::uint8_t>& encodedBytes() const noexcept;

  private:
    std::vector<std::uint8_t> m_encodedBytes; ///< Raw encoded audio data.
    std::string m_formatName;                 ///< Format identifier (e.g. "mp3").
};
} // namespace engine
