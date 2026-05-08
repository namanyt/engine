#include "assets/AssetRegistry.h"

#include <algorithm>
#include <cctype>
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

std::size_t AssetRegistry::discover(const std::filesystem::path& rootPath)
{
    if (!std::filesystem::exists(rootPath))
    {
        return 0;
    }

    std::size_t discoveredCount = 0;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(rootPath))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        if (entry.path().extension() == ".meta")
        {
            continue;
        }

        const AssetType inferredType = inferAssetTypeFromPath(entry.path());
        if (!isLoadableBaseAssetType(inferredType))
        {
            continue;
        }

        registerAsset(entry.path());
        ++discoveredCount;
    }

    return discoveredCount;
}

AssetHandle<> AssetRegistry::registerAsset(const std::filesystem::path& assetPath)
{
    const AssetType inferredType = inferAssetTypeFromPath(assetPath);
    if (!isLoadableBaseAssetType(inferredType))
    {
        return {};
    }

    const std::filesystem::path normalizedPath = normalizePath(assetPath);
    const std::string pathKey = makePathKey(normalizedPath);

    const auto existingPath = m_uuidByPath.find(pathKey);
    if (existingPath != m_uuidByPath.end())
    {
        const Record* existingRecord = findByUuid(existingPath->second);
        return existingRecord != nullptr
                   ? AssetHandle<>(existingRecord->meta.uuid, existingRecord->meta.type)
                   : AssetHandle<>();
    }

    const std::filesystem::path metaPath = normalizedPath.string() + ".meta";
    const bool hasMetaFile = std::filesystem::exists(metaPath);

    Record record{};
    record.assetPath = normalizedPath;
    record.metaPath = metaPath;
    record.hasMetaFile = hasMetaFile;
    record.meta = hasMetaFile ? loadAssetMeta(metaPath, normalizedPath, inferredType)
                              : makeDefaultAssetMeta(normalizedPath, inferredType);
    record.meta.sourcePath = normalizedPath;

    if (!hasMetaFile)
    {
        saveAssetMeta(metaPath, record.meta);
        record.hasMetaFile = true;
        record.generatedMetaFile = true;
    }

    const auto existingUuid = m_recordsByUuid.find(record.meta.uuid);
    if (existingUuid != m_recordsByUuid.end())
    {
        throw std::runtime_error("Duplicate asset UUID detected for '" +
                                 existingUuid->second.assetPath.string() + "' and '" +
                                 normalizedPath.string() + "': " + record.meta.uuid);
    }

    const std::string uuid = record.meta.uuid;
    const AssetType type = record.meta.type;

    m_uuidByPath.emplace(pathKey, uuid);
    m_recordsByUuid.emplace(uuid, std::move(record));
    return AssetHandle<>(uuid, type);
}

const AssetRegistry::Record* AssetRegistry::findByUuid(const std::string& uuid) const noexcept
{
    const auto iterator = m_recordsByUuid.find(uuid);
    return iterator != m_recordsByUuid.end() ? &iterator->second : nullptr;
}

const AssetRegistry::Record*
AssetRegistry::findByPath(const std::filesystem::path& assetPath) const noexcept
{
    const auto uuidIterator = m_uuidByPath.find(makePathKey(normalizePath(assetPath)));
    if (uuidIterator == m_uuidByPath.end())
    {
        return nullptr;
    }

    return findByUuid(uuidIterator->second);
}

std::vector<AssetHandle<>> AssetRegistry::preloadHandles() const
{
    std::vector<AssetHandle<>> handles;
    handles.reserve(m_recordsByUuid.size());

    for (const auto& [uuid, record] : m_recordsByUuid)
    {
        if (record.meta.preload)
        {
            handles.emplace_back(uuid, record.meta.type);
        }
    }

    return handles;
}

const std::unordered_map<std::string, AssetRegistry::Record>&
AssetRegistry::records() const noexcept
{
    return m_recordsByUuid;
}

std::size_t AssetRegistry::count(AssetType type) const noexcept
{
    std::size_t assetCount = 0;
    for (const auto& [uuid, record] : m_recordsByUuid)
    {
        (void)uuid;
        if (record.meta.type == type)
        {
            ++assetCount;
        }
    }

    return assetCount;
}

std::filesystem::path AssetRegistry::normalizePath(const std::filesystem::path& path)
{
    std::error_code error;
    const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, error);
    if (!error)
    {
        return canonicalPath.lexically_normal();
    }

    return std::filesystem::absolute(path).lexically_normal();
}

std::string AssetRegistry::makePathKey(const std::filesystem::path& path)
{
    std::string key = path.generic_string();
#if defined(_WIN32)
    key = toLower(key);
#endif
    return key;
}
} // namespace engine
