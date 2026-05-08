#include "assets/AssetMeta.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace engine
{
namespace
{
std::string trim(std::string value)
{
    const auto isWhitespace = [](unsigned char character) { return std::isspace(character) != 0; };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char character)
                                            { return !isWhitespace(character); }));

    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [&](unsigned char character) { return !isWhitespace(character); })
                    .base(),
                value.end());

    return value;
}

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                   { return static_cast<char>(std::tolower(character)); });
    return value;
}

std::string fileTimestampString(const std::filesystem::path& path)
{
    namespace chrono = std::chrono;

    std::error_code error;
    const auto fileTime = std::filesystem::last_write_time(path, error);
    if (error)
    {
        return {};
    }

    const auto systemTime = chrono::time_point_cast<chrono::system_clock::duration>(
        fileTime - std::filesystem::file_time_type::clock::now() + chrono::system_clock::now());
    const std::time_t timestamp = chrono::system_clock::to_time_t(systemTime);

    std::tm utcTime{};
#if defined(_WIN32)
    gmtime_s(&utcTime, &timestamp);
#else
    gmtime_r(&timestamp, &utcTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

std::vector<std::string> parseTags(const std::string& value)
{
    std::vector<std::string> tags;
    std::stringstream stream(value);
    std::string item;

    while (std::getline(stream, item, ','))
    {
        item = trim(item);
        if (!item.empty())
        {
            tags.push_back(std::move(item));
        }
    }

    return tags;
}

bool parseBool(const std::string& value)
{
    const std::string lowered = toLower(trim(value));
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
}

std::filesystem::path pathForMetaFile(const std::filesystem::path& metaPath,
                                      const std::filesystem::path& sourcePath)
{
    std::error_code error;
    const std::filesystem::path relativePath =
        std::filesystem::relative(sourcePath, metaPath.parent_path(), error);
    if (!error)
    {
        return relativePath.lexically_normal();
    }

    return sourcePath.lexically_normal();
}

std::string joinTags(const std::vector<std::string>& tags)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < tags.size(); ++index)
    {
        if (index != 0)
        {
            stream << ", ";
        }

        stream << tags[index];
    }

    return stream.str();
}
} // namespace

const char* assetTypeName(AssetType type) noexcept
{
    switch (type)
    {
    case AssetType::Texture:
        return "texture";
    case AssetType::Audio:
        return "audio";
    case AssetType::Model:
        return "model";
    case AssetType::Shader:
        return "shader";
    case AssetType::Font:
        return "font";
    case AssetType::Video:
        return "video";
    default:
        return "unknown";
    }
}

AssetType assetTypeFromString(std::string_view value) noexcept
{
    const std::string lowered = toLower(trim(std::string(value)));

    if (lowered == "texture")
    {
        return AssetType::Texture;
    }

    if (lowered == "audio")
    {
        return AssetType::Audio;
    }

    if (lowered == "model")
    {
        return AssetType::Model;
    }

    if (lowered == "shader")
    {
        return AssetType::Shader;
    }

    if (lowered == "font")
    {
        return AssetType::Font;
    }

    if (lowered == "video")
    {
        return AssetType::Video;
    }

    return AssetType::Unknown;
}

AssetType inferAssetTypeFromPath(const std::filesystem::path& path) noexcept
{
    const std::string extension = toLower(path.extension().string());

    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".hdr")
    {
        return AssetType::Texture;
    }

    if (extension == ".wav" || extension == ".mp3")
    {
        return AssetType::Audio;
    }

    if (extension == ".obj" || extension == ".gltf")
    {
        return AssetType::Model;
    }

    if (extension == ".vert" || extension == ".frag" || extension == ".glsl")
    {
        return AssetType::Shader;
    }

    if (extension == ".ttf")
    {
        return AssetType::Font;
    }

    if (extension == ".mp4" || extension == ".mov")
    {
        return AssetType::Video;
    }

    return AssetType::Unknown;
}

bool isLoadableBaseAssetType(AssetType type) noexcept
{
    return type != AssetType::Unknown;
}

ShaderStage inferShaderStageFromPath(const std::filesystem::path& path) noexcept
{
    const std::string extension = toLower(path.extension().string());
    if (extension == ".vert")
    {
        return ShaderStage::Vertex;
    }

    if (extension == ".frag")
    {
        return ShaderStage::Fragment;
    }

    if (extension == ".glsl")
    {
        return ShaderStage::Unknown;
    }

    return ShaderStage::Unknown;
}

std::string generateAssetUuid()
{
    static std::random_device seedDevice;
    static std::mt19937_64 generator(seedDevice());
    static std::uniform_int_distribution<unsigned int> distribution(0U, 15U);
    static constexpr std::array<int, 5> segmentSizes = {8, 4, 4, 4, 12};

    std::ostringstream stream;
    stream << std::hex << std::nouppercase;

    for (std::size_t segmentIndex = 0; segmentIndex < segmentSizes.size(); ++segmentIndex)
    {
        if (segmentIndex != 0)
        {
            stream << '-';
        }

        for (int characterIndex = 0; characterIndex < segmentSizes[segmentIndex]; ++characterIndex)
        {
            stream << distribution(generator);
        }
    }

    return stream.str();
}

AssetMeta makeDefaultAssetMeta(const std::filesystem::path& assetPath, AssetType type)
{
    AssetMeta meta{};
    meta.uuid = generateAssetUuid();
    meta.type = type;
    meta.sourcePath = assetPath;
    meta.importTimestamp = fileTimestampString(assetPath);
    return meta;
}

AssetMeta loadAssetMeta(const std::filesystem::path& metaPath,
                        const std::filesystem::path& assetPath, AssetType inferredType)
{
    AssetMeta meta = makeDefaultAssetMeta(assetPath, inferredType);

    std::ifstream file(metaPath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open asset meta file: " + metaPath.string());
    }

    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line.front() == '#')
        {
            continue;
        }

        const std::size_t delimiter = line.find('=');
        if (delimiter == std::string::npos)
        {
            continue;
        }

        const std::string key = toLower(trim(line.substr(0, delimiter)));
        const std::string value = trim(line.substr(delimiter + 1));

        if (key == "uuid")
        {
            if (!value.empty())
            {
                meta.uuid = value;
            }
        }
        else if (key == "asset_type" || key == "type")
        {
            const AssetType parsedType = assetTypeFromString(value);
            if (parsedType != AssetType::Unknown)
            {
                meta.type = parsedType;
            }
        }
        else if (key == "original_path" || key == "path" || key == "source_path")
        {
            if (!value.empty())
            {
                meta.sourcePath = value;
            }
        }
        else if (key == "import_timestamp" || key == "imported_at")
        {
            meta.importTimestamp = value;
        }
        else if (key == "tags")
        {
            meta.tags = parseTags(value);
        }
        else if (key == "preload")
        {
            meta.preload = parseBool(value);
        }
    }

    if (meta.uuid.empty())
    {
        meta.uuid = generateAssetUuid();
    }

    if (meta.type == AssetType::Unknown)
    {
        meta.type = inferredType;
    }

    if (meta.sourcePath.empty())
    {
        meta.sourcePath = assetPath;
    }
    else if (meta.sourcePath.is_relative())
    {
        meta.sourcePath = (metaPath.parent_path() / meta.sourcePath).lexically_normal();
    }

    if (meta.importTimestamp.empty())
    {
        meta.importTimestamp = fileTimestampString(assetPath);
    }

    return meta;
}

void saveAssetMeta(const std::filesystem::path& metaPath, const AssetMeta& meta)
{
    std::filesystem::create_directories(metaPath.parent_path());

    std::ofstream file(metaPath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to write asset meta file: " + metaPath.string());
    }

    file << "uuid=" << meta.uuid << '\n';
    file << "asset_type=" << assetTypeName(meta.type) << '\n';
    file << "source_path=" << pathForMetaFile(metaPath, meta.sourcePath).generic_string() << '\n';
    file << "import_timestamp=" << meta.importTimestamp << '\n';
    file << "tags=" << joinTags(meta.tags) << '\n';
    file << "preload=" << (meta.preload ? "true" : "false") << '\n';
}
} // namespace engine
