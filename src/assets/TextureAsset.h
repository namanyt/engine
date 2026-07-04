#pragma once

#include "assets/Asset.h"

#include <memory>

namespace engine
{
/**
 * @brief Loaded texture asset with GPU texture ID and dimensions.
 *
 * Wraps an OpenGL texture handle along with metadata about the source image
 * (dimensions, channel count, HDR flag).
 *
 * @see Asset
 * @see AssetManager
 */
class TextureAsset final : public Asset
{
  public:
    /// @brief Factory: loads a texture from disk and uploads it to the GPU.
    /// @param meta Asset metadata containing the source path.
    /// @return Shared pointer to the loaded TextureAsset, or nullptr on failure.
    static std::shared_ptr<TextureAsset> loadFromFile(const AssetMeta& meta);

    /// @brief Constructs a texture asset with pre-loaded GPU data.
    /// @param meta Asset metadata.
    /// @param textureId OpenGL texture handle.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    /// @param channelCount Number of color channels (3=RGB, 4=RGBA).
    /// @param hdr Whether the texture uses high-dynamic-range encoding.
    TextureAsset(AssetMeta meta, unsigned int textureId, int width, int height, int channelCount,
                 bool hdr);

    /// @brief Destroys the GPU texture resource.
    ~TextureAsset() override;

    /// @brief Returns the OpenGL texture handle.
    /// @return Unsigned integer texture ID for use with GL calls.
    unsigned int textureId() const noexcept;

    /// @brief Returns the texture width in pixels.
    /// @return Width as an integer.
    int width() const noexcept;

    /// @brief Returns the texture height in pixels.
    /// @return Height as an integer.
    int height() const noexcept;

    /// @brief Returns the number of color channels (3=RGB, 4=RGBA).
    /// @return Channel count as an integer.
    int channelCount() const noexcept;

    /// @brief Checks whether this texture uses HDR encoding.
    /// @return true if the texture is high-dynamic-range.
    bool isHdr() const noexcept;

  private:
    unsigned int m_textureId = 0; ///< OpenGL texture handle.
    int m_width = 0;              ///< Texture width in pixels.
    int m_height = 0;             ///< Texture height in pixels.
    int m_channelCount = 0;       ///< Number of color channels.
    bool m_isHdr = false;         ///< HDR encoding flag.
};
} // namespace engine
