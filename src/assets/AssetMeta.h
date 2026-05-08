#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace engine
{
enum class AssetType
{
    Unknown,
    Texture,
    Audio,
    Model,
    Shader,
    Font,
    Video
};

enum class ShaderStage
{
    Unknown,
    Vertex,
    Fragment
};

struct AssetMeta final
{
    std::string uuid;
    AssetType type = AssetType::Unknown;
    std::filesystem::path sourcePath;
    std::string importTimestamp;
    std::vector<std::string> tags;
    bool preload = false;
};

const char* assetTypeName(AssetType type) noexcept;
AssetType assetTypeFromString(std::string_view value) noexcept;
AssetType inferAssetTypeFromPath(const std::filesystem::path& path) noexcept;
bool isLoadableBaseAssetType(AssetType type) noexcept;
ShaderStage inferShaderStageFromPath(const std::filesystem::path& path) noexcept;

std::string generateAssetUuid();
AssetMeta makeDefaultAssetMeta(const std::filesystem::path& assetPath, AssetType type);
AssetMeta loadAssetMeta(const std::filesystem::path& metaPath,
                        const std::filesystem::path& assetPath, AssetType inferredType);
void saveAssetMeta(const std::filesystem::path& metaPath, const AssetMeta& meta);
} // namespace engine
