#pragma once

#include "assets/Asset.h"
#include "assets/AudioAsset.h"
#include "assets/AssetHandle.h"
#include "assets/ModelAsset.h"
#include "assets/AssetRegistry.h"
#include "assets/ShaderAsset.h"
#include "assets/TextureAsset.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace engine
{
/// @brief Hash function object for `AssetType` enum values.
struct AssetTypeHash final
{
    std::size_t operator()(AssetType type) const noexcept
    {
        return static_cast<std::size_t>(type);
    }
};

/**
 * @brief Central manager for asset discovery, loading, caching, and unloading.
 *
 * `AssetManager` scans a directory tree for assets, builds a registry of
 * discovered files, and provides type-safe loading with an LRU-style cache.
 * Assets are loaded on-demand via `load<T>()` and cached as `weak_ptr` entries.
 *
 * @par Example
 * @code
 * AssetManager manager;
 * manager.discover("assets/");
 *
 * // Load by path
 * auto texture = manager.load<TextureAsset>("textures/background.png.meta");
 *
 * // Load by UUID
 * auto audio = manager.load<AudioAsset>("550e8400-e29b-41d4-a716-446655440000");
 * @endcode
 *
 * @see AssetRegistry
 * @see AssetHandle
 * @see AssetMeta
 */
class AssetManager final
{
  public:
    /// @brief Loader function type: takes a registry record and returns a loaded asset.
    using Loader = std::function<std::shared_ptr<Asset>(const AssetRegistry::Record& record)>;

    /// @brief Constructs an empty AssetManager with no discovered assets.
    AssetManager();

    /// @brief Discovers all loadable assets under the given root directory.
    /// @param rootPath Filesystem path to scan for assets.
    /// @return Number of assets discovered and registered.
    std::size_t discover(const std::filesystem::path& rootPath);

    /// @brief Registers a single asset file in the registry.
    /// @param assetPath Path to the asset file.
    /// @return Handle referencing the registered asset.
    AssetHandle<> registerAsset(const std::filesystem::path& assetPath);

    /// @brief Resolves a relative asset path against the root directory.
    /// @param assetPath Relative or absolute path to resolve.
    /// @return Fully qualified filesystem path.
    std::filesystem::path resolveAssetPath(const std::filesystem::path& assetPath) const;

    /// @brief Returns the current asset root directory.
    /// @return Reference to the root directory path.
    const std::filesystem::path& assetRootDirectory() const noexcept;

    /// @brief Finds an asset handle by UUID.
    /// @tparam TAsset Desired asset type (default: `Asset`).
    /// @param uuid Unique identifier string.
    /// @return Type-safe handle, or invalid handle if not found or type mismatch.
    template <typename TAsset = Asset> AssetHandle<TAsset> findByUuid(const std::string& uuid) const
    {
        const AssetRegistry::Record* record = m_registry.findByUuid(uuid);
        return typedHandle<TAsset>(record);
    }

    /// @brief Finds an asset handle by filesystem path.
    /// @tparam TAsset Desired asset type (default: `Asset`).
    /// @param assetPath Relative or absolute path to the asset.
    /// @return Type-safe handle, or invalid handle if not found or type mismatch.
    template <typename TAsset = Asset>
    AssetHandle<TAsset> findByPath(const std::filesystem::path& assetPath) const
    {
        const AssetRegistry::Record* record = m_registry.findByPath(resolveAssetPath(assetPath));
        return typedHandle<TAsset>(record);
    }

    /// @brief Loads an asset from an untyped handle, using the cache if available.
    /// @param handle Handle referencing the asset to load.
    /// @return Shared pointer to the loaded base Asset.
    std::shared_ptr<Asset> load(const AssetHandle<>& handle);

    /// @brief Loads a typed asset from a type-safe handle.
    /// @tparam TAsset Desired asset type (e.g. `TextureAsset`).
    /// @param handle Type-safe handle referencing the asset.
    /// @return Shared pointer to the loaded typed asset, or nullptr on cast failure.
    template <typename TAsset> std::shared_ptr<TAsset> load(const AssetHandle<TAsset>& handle)
    {
        const std::shared_ptr<Asset> asset = load(handle.untyped());
        return std::dynamic_pointer_cast<TAsset>(asset);
    }

    /// @brief Loads a typed asset by UUID string (convenience overload).
    /// @tparam TAsset Desired asset type.
    /// @param uuid Unique identifier string.
    /// @return Shared pointer to the loaded typed asset.
    template <typename TAsset> std::shared_ptr<TAsset> load(const std::string& uuid)
    {
        return load(findByUuid<TAsset>(uuid));
    }

    /// @brief Loads a typed asset by filesystem path (convenience overload).
    /// @tparam TAsset Desired asset type.
    /// @param assetPath Relative or absolute path to the asset.
    /// @return Shared pointer to the loaded typed asset.
    template <typename TAsset> std::shared_ptr<TAsset> load(const std::filesystem::path& assetPath)
    {
        return load(findByPath<TAsset>(assetPath));
    }

    /// @brief Checks whether an asset is currently loaded in the cache.
    /// @param uuid Unique identifier string.
    /// @return true if a live cached entry exists for this UUID.
    bool isLoaded(const std::string& uuid) const;

    /// @brief Reloads an asset from disk, bypassing the cache.
    /// @param uuid Unique identifier string.
    /// @return true if the asset was successfully reloaded.
    bool reload(const std::string& uuid);

    /// @brief Unloads an asset from the cache.
    /// @param uuid Unique identifier string.
    void unload(const std::string& uuid);

    /// @brief Unloads all cached assets (excluding pinned preloads).
    void unloadAll();

    /// @brief Returns a reference to the underlying asset registry.
    /// @return Const reference to the `AssetRegistry`.
    const AssetRegistry& registry() const noexcept;

    /// @brief Returns the number of registered assets of a specific type.
    /// @param type Asset type to count.
    /// @return Number of assets with the given type.
    std::size_t count(AssetType type) const noexcept;

    /// @brief Returns the total number of registered assets across all types.
    /// @return Total asset count in the registry.
    std::size_t totalCount() const noexcept;

  private:
    template <typename TAsset>
    static AssetHandle<TAsset> typedHandle(const AssetRegistry::Record* record)
    {
        if (record == nullptr)
        {
            return {};
        }

        const AssetType requestedType = AssetTypeTraits<TAsset>::type;
        if (requestedType != AssetType::Unknown && record->meta.type != requestedType)
        {
            return {};
        }

        return AssetHandle<TAsset>(record->meta.uuid, record->meta.type);
    }

    std::shared_ptr<Asset> loadRecord(const AssetRegistry::Record& record);
    void registerLoader(AssetType type, Loader loader);

    AssetRegistry m_registry;
    std::filesystem::path m_assetRootDirectory;
    std::unordered_map<AssetType, Loader, AssetTypeHash> m_loaders;
    std::unordered_map<std::string, std::weak_ptr<Asset>> m_cache;
    std::unordered_map<std::string, std::shared_ptr<Asset>> m_pinnedPreloads;
};
} // namespace engine
