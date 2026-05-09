#include "assets/AssetManager.h"

#include "core/Log.h"

#include <array>
#include <sstream>
#include <unordered_set>

namespace engine
{
namespace
{
std::filesystem::path normalizeFilesystemPath(const std::filesystem::path& path)
{
    std::error_code error;
    const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, error);
    if (!error)
    {
        return canonicalPath.lexically_normal();
    }

    return std::filesystem::absolute(path).lexically_normal();
}

const char* assetClassName(AssetType type) noexcept
{
    switch (type)
    {
    case AssetType::Texture:
        return "TextureAsset";
    case AssetType::Audio:
        return "AudioAsset";
    case AssetType::Model:
        return "ModelAsset";
    case AssetType::Shader:
        return "ShaderAsset";
    case AssetType::Font:
        return "FontAsset";
    case AssetType::Video:
        return "VideoAsset";
    default:
        return "Asset";
    }
}
} // namespace

AssetManager::AssetManager()
{
    registerLoader(AssetType::Texture, [](const AssetRegistry::Record& record)
                   { return TextureAsset::loadFromFile(record.meta); });
    registerLoader(AssetType::Audio, [](const AssetRegistry::Record& record)
                   { return AudioAsset::loadFromFile(record.meta); });
    registerLoader(AssetType::Model, [](const AssetRegistry::Record& record)
                   { return ModelAsset::loadFromFile(record.meta); });
    registerLoader(AssetType::Shader, [](const AssetRegistry::Record& record)
                   { return ShaderAsset::loadFromFile(record.meta); });
}

std::size_t AssetManager::discover(const std::filesystem::path& rootPath)
{
    m_assetRootDirectory = normalizeFilesystemPath(rootPath);

    std::unordered_set<std::string> existingUuids;
    existingUuids.reserve(m_registry.records().size());
    for (const auto& [uuid, record] : m_registry.records())
    {
        (void)record;
        existingUuids.insert(uuid);
    }

    const std::size_t discoveredCount = m_registry.discover(rootPath);

    for (const auto& [uuid, record] : m_registry.records())
    {
        if (existingUuids.find(uuid) != existingUuids.end())
        {
            continue;
        }

        std::ostringstream stream;
        stream << "Registered " << assetClassName(record.meta.type) << ": "
               << record.assetPath.filename().string();
        Log::info("Assets", stream.str());

        if (record.generatedMetaFile)
        {
            std::ostringstream metaStream;
            metaStream << "Generated metadata: " << record.metaPath.filename().string();
            Log::info("Assets", metaStream.str());
        }
    }

    for (const AssetHandle<>& handle : m_registry.preloadHandles())
    {
        if (std::shared_ptr<Asset> asset = load(handle))
        {
            m_pinnedPreloads[handle.uuid()] = std::move(asset);
        }
    }

    for (const AssetType type : {AssetType::Texture, AssetType::Audio, AssetType::Model,
                                 AssetType::Shader, AssetType::Font, AssetType::Video})
    {
        const std::size_t assetCount = count(type);
        if (assetCount == 0)
        {
            continue;
        }

        std::ostringstream stream;
        stream << assetTypeName(type) << " count: " << assetCount;
        Log::info("Assets", stream.str());
    }

    {
        std::ostringstream stream;
        stream << "Shader registration state: " << count(AssetType::Shader)
               << " shader assets registered.";
        Log::info("Assets", stream.str());
    }

    return discoveredCount;
}

AssetHandle<> AssetManager::registerAsset(const std::filesystem::path& assetPath)
{
    const AssetHandle<> handle = m_registry.registerAsset(resolveAssetPath(assetPath));
    if (handle && m_registry.findByUuid(handle.uuid()) != nullptr)
    {
        const AssetRegistry::Record& record = *m_registry.findByUuid(handle.uuid());
        if (record.meta.preload)
        {
            m_pinnedPreloads[handle.uuid()] = load(handle);
        }
    }

    return handle;
}

std::filesystem::path AssetManager::resolveAssetPath(const std::filesystem::path& assetPath) const
{
    if (assetPath.empty())
    {
        return {};
    }

    if (assetPath.is_absolute())
    {
        return normalizeFilesystemPath(assetPath);
    }

    if (!m_assetRootDirectory.empty())
    {
        return (m_assetRootDirectory / assetPath).lexically_normal();
    }

    return assetPath.lexically_normal();
}

const std::filesystem::path& AssetManager::assetRootDirectory() const noexcept
{
    return m_assetRootDirectory;
}

std::shared_ptr<Asset> AssetManager::load(const AssetHandle<>& handle)
{
    if (!handle)
    {
        return nullptr;
    }

    const auto cached = m_cache.find(handle.uuid());
    if (cached != m_cache.end())
    {
        if (std::shared_ptr<Asset> asset = cached->second.lock())
        {
            return asset;
        }
    }

    const AssetRegistry::Record* record = m_registry.findByUuid(handle.uuid());
    if (record == nullptr)
    {
        return nullptr;
    }

    return loadRecord(*record);
}

bool AssetManager::isLoaded(const std::string& uuid) const
{
    const auto iterator = m_cache.find(uuid);
    return iterator != m_cache.end() && !iterator->second.expired();
}

bool AssetManager::reload(const std::string& uuid)
{
    unload(uuid);
    return load(findByUuid(uuid)) != nullptr;
}

void AssetManager::unload(const std::string& uuid)
{
    m_pinnedPreloads.erase(uuid);
    m_cache.erase(uuid);
}

void AssetManager::unloadAll()
{
    m_pinnedPreloads.clear();
    m_cache.clear();
}

const AssetRegistry& AssetManager::registry() const noexcept
{
    return m_registry;
}

std::size_t AssetManager::count(AssetType type) const noexcept
{
    return m_registry.count(type);
}

std::size_t AssetManager::totalCount() const noexcept
{
    return m_registry.records().size();
}

std::shared_ptr<Asset> AssetManager::loadRecord(const AssetRegistry::Record& record)
{
    const auto loader = m_loaders.find(record.meta.type);
    if (loader == m_loaders.end())
    {
        throw std::runtime_error("No asset loader registered for type '" +
                                 std::string(assetTypeName(record.meta.type)) + "'.");
    }

    std::shared_ptr<Asset> asset = loader->second(record);
    m_cache[record.meta.uuid] = asset;
    return asset;
}

void AssetManager::registerLoader(AssetType type, Loader loader)
{
    m_loaders[type] = std::move(loader);
}
} // namespace engine
