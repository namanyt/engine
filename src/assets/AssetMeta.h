#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace engine
{
/// @brief Classification of asset types recognized by the engine.
enum class AssetType
{
    Unknown, ///< Unrecognized or unclassified asset.
    Texture, ///< Image file (PNG, JPG, etc.).
    Audio,   ///< Sound or music file (MP3, WAV, OGG).
    Model,   ///< 3D model file (OBJ, GLTF).
    Shader,  ///< GLSL shader source file.
    Font,    ///< Font file (TTF, OTF).
    Video    ///< Video file (MP4, WebM).
};

/// @brief Shader compilation stage inferred from filename or metadata.
enum class ShaderStage
{
    Unknown, ///< Could not determine the shader stage.
    Vertex,  ///< Vertex shader (.vert / .glsl vertex).
    Fragment ///< Fragment shader (.frag / .glsl fragment).
};

/// @brief Metadata record for a discovered asset.
///
/// Contains the UUID, type, source path, and optional tags for an asset.
/// Persisted as a `.meta` file alongside the asset on disk.
struct AssetMeta final
{
    std::string uuid;                    ///< Unique identifier (UUID v4).
    AssetType type = AssetType::Unknown; ///< Classification of this asset.
    std::filesystem::path sourcePath;    ///< Relative path to the asset file.
    std::string importTimestamp;         ///< ISO-8601 timestamp of last import.
    std::vector<std::string> tags;       ///< User-defined tags for organization.
    bool preload = false;                ///< Whether this asset should be preloaded at startup.
};

/// @brief Returns the human-readable name for an asset type.
/// @param type The asset type enum value.
/// @return Null-terminated string (e.g. "Texture", "Audio").
const char* assetTypeName(AssetType type) noexcept;

/// @brief Parses an asset type from a string representation.
/// @param value Lowercase or title-case type name (e.g. "texture", "Texture").
/// @return Corresponding AssetType, or AssetType::Unknown if not recognized.
AssetType assetTypeFromString(std::string_view value) noexcept;

/// @brief Infers the asset type from a file extension.
/// @param path Filesystem path to the asset.
/// @return Best-guess AssetType based on the file extension.
AssetType inferAssetTypeFromPath(const std::filesystem::path& path) noexcept;

/// @brief Checks whether an asset type can be loaded as a base resource.
/// @param type The asset type to check.
/// @return true if the type is loadable (not Unknown).
bool isLoadableBaseAssetType(AssetType type) noexcept;

/// @brief Infers the shader stage from a filename.
/// @param path Filesystem path to the shader file.
/// @return ShaderStage::Vertex for .vert files, Fragment for .frag files, Unknown otherwise.
ShaderStage inferShaderStageFromPath(const std::filesystem::path& path) noexcept;

/// @brief Generates a new random UUID string (UUID v4 format).
/// @return A 36-character UUID string (e.g. "550e8400-e29b-41d4-a716-446655440000").
std::string generateAssetUuid();

/// @brief Creates a default AssetMeta for a given asset path and type.
/// @param assetPath Relative or absolute path to the asset file.
/// @param type The asset type classification.
/// @return A new AssetMeta with a generated UUID and default values.
AssetMeta makeDefaultAssetMeta(const std::filesystem::path& assetPath, AssetType type);

/// @brief Loads AssetMeta from a .meta file on disk.
/// @param metaPath Path to the .meta file.
/// @param assetPath Path to the associated asset file.
/// @param inferredType Fallback type if the meta file does not specify one.
/// @return Parsed AssetMeta with values from the file.
AssetMeta loadAssetMeta(const std::filesystem::path& metaPath,
                        const std::filesystem::path& assetPath, AssetType inferredType);

/// @brief Saves AssetMeta to a .meta file on disk.
/// @param metaPath Destination path for the .meta file.
/// @param meta Metadata record to persist.
void saveAssetMeta(const std::filesystem::path& metaPath, const AssetMeta& meta);
} // namespace engine
