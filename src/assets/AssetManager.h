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
struct AssetTypeHash final
{
    std::size_t operator()(AssetType type) const noexcept
    {
        return static_cast<std::size_t>(type);
    }
};

class AssetManager final
{
  public:
    using Loader = std::function<std::shared_ptr<Asset>(const AssetRegistry::Record& record)>;

    AssetManager();

    std::size_t discover(const std::filesystem::path& rootPath);
    AssetHandle<> registerAsset(const std::filesystem::path& assetPath);

    template <typename TAsset = Asset> AssetHandle<TAsset> findByUuid(const std::string& uuid) const
    {
        const AssetRegistry::Record* record = m_registry.findByUuid(uuid);
        return typedHandle<TAsset>(record);
    }

    template <typename TAsset = Asset>
    AssetHandle<TAsset> findByPath(const std::filesystem::path& assetPath) const
    {
        const AssetRegistry::Record* record = m_registry.findByPath(assetPath);
        return typedHandle<TAsset>(record);
    }

    std::shared_ptr<Asset> load(const AssetHandle<>& handle);

    template <typename TAsset> std::shared_ptr<TAsset> load(const AssetHandle<TAsset>& handle)
    {
        const std::shared_ptr<Asset> asset = load(handle.untyped());
        return std::dynamic_pointer_cast<TAsset>(asset);
    }

    template <typename TAsset> std::shared_ptr<TAsset> load(const std::string& uuid)
    {
        return load(findByUuid<TAsset>(uuid));
    }

    template <typename TAsset> std::shared_ptr<TAsset> load(const std::filesystem::path& assetPath)
    {
        return load(findByPath<TAsset>(assetPath));
    }

    bool isLoaded(const std::string& uuid) const;
    bool reload(const std::string& uuid);
    void unload(const std::string& uuid);
    void unloadAll();

    const AssetRegistry& registry() const noexcept;
    std::size_t count(AssetType type) const noexcept;
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
    std::unordered_map<AssetType, Loader, AssetTypeHash> m_loaders;
    std::unordered_map<std::string, std::weak_ptr<Asset>> m_cache;
    std::unordered_map<std::string, std::shared_ptr<Asset>> m_pinnedPreloads;
};
} // namespace engine
