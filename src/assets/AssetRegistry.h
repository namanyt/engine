#pragma once

#include "assets/AssetHandle.h"
#include "assets/AssetMeta.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine
{
class AssetRegistry final
{
  public:
    struct Record final
    {
        AssetMeta meta;
        std::filesystem::path assetPath;
        std::filesystem::path metaPath;
        bool hasMetaFile = false;
        bool generatedMetaFile = false;
    };

    std::size_t discover(const std::filesystem::path& rootPath);
    AssetHandle<> registerAsset(const std::filesystem::path& assetPath);

    const Record* findByUuid(const std::string& uuid) const noexcept;
    const Record* findByPath(const std::filesystem::path& assetPath) const noexcept;
    std::vector<AssetHandle<>> preloadHandles() const;
    const std::unordered_map<std::string, Record>& records() const noexcept;
    std::size_t count(AssetType type) const noexcept;

  private:
    static std::filesystem::path normalizePath(const std::filesystem::path& path);
    static std::string makePathKey(const std::filesystem::path& path);

    std::unordered_map<std::string, Record> m_recordsByUuid;
    std::unordered_map<std::string, std::string> m_uuidByPath;
};
} // namespace engine
