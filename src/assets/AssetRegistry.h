#pragma once

#include "assets/AssetHandle.h"
#include "assets/AssetMeta.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine
{
/**
 * @brief Registry of discovered assets with UUID and path lookups.
 *
 * `AssetRegistry` maintains a bidirectional index between asset UUIDs and
 * filesystem paths. It is populated by scanning a directory tree and reading
 * `.meta` companion files.
 *
 * @see AssetManager
 * @see AssetMeta
 */
class AssetRegistry final
{
  public:
    /// @brief Complete record for a single discovered asset.
    struct Record final
    {
        AssetMeta meta;                  ///< Metadata (UUID, type, tags).
        std::filesystem::path assetPath; ///< Normalized path to the asset file.
        std::filesystem::path metaPath;  ///< Path to the companion .meta file.
        bool hasMetaFile = false;        ///< Whether a .meta file was found on disk.
        bool generatedMetaFile = false;  ///< Whether the .meta file was auto-generated.
    };

    /// @brief Discovers all loadable assets under the given root directory.
    /// @param rootPath Filesystem path to scan recursively.
    /// @return Number of new assets discovered and registered.
    std::size_t discover(const std::filesystem::path& rootPath);

    /// @brief Registers a single asset file in the registry.
    /// @param assetPath Path to the asset file.
    /// @return Handle referencing the registered asset.
    AssetHandle<> registerAsset(const std::filesystem::path& assetPath);

    /// @brief Looks up an asset record by UUID.
    /// @param uuid Unique identifier string.
    /// @return Pointer to the Record, or nullptr if not found.
    const Record* findByUuid(const std::string& uuid) const noexcept;

    /// @brief Looks up an asset record by filesystem path.
    /// @param assetPath Normalized path to the asset file.
    /// @return Pointer to the Record, or nullptr if not found.
    const Record* findByPath(const std::filesystem::path& assetPath) const noexcept;

    /// @brief Returns handles for all assets marked as preload.
    /// @return Vector of untyped handles for preloaded assets.
    std::vector<AssetHandle<>> preloadHandles() const;

    /// @brief Returns the complete map of records indexed by UUID.
    /// @return Const reference to the internal UUID-to-Record map.
    const std::unordered_map<std::string, Record>& records() const noexcept;

    /// @brief Returns the number of registered assets of a specific type.
    /// @param type Asset type to count.
    /// @return Number of assets with the given type.
    std::size_t count(AssetType type) const noexcept;

  private:
    static std::filesystem::path normalizePath(const std::filesystem::path& path);
    static std::string makePathKey(const std::filesystem::path& path);

    std::unordered_map<std::string, Record> m_recordsByUuid;
    std::unordered_map<std::string, std::string> m_uuidByPath;
};
} // namespace engine
