#include "core/ShaderLibrary.h"

#include "assets/AssetManager.h"
#include "assets/ShaderAsset.h"
#include "core/Log.h"
#include "core/Shader.h"

#include <sstream>

namespace engine
{
ShaderLibrary::ShaderLibrary(std::shared_ptr<AssetManager> assetManager,
                             std::filesystem::path shaderDirectory)
    : m_assetManager(std::move(assetManager)), m_shaderDirectory(std::move(shaderDirectory))
{
}

const Shader& ShaderLibrary::loadGraphicsProgram(const std::string& key,
                                                 const std::string& vertexShaderName,
                                                 const std::string& fragmentShaderName)
{
    return loadGraphicsProgram(key, std::filesystem::path(vertexShaderName),
                               std::filesystem::path(fragmentShaderName));
}

const Shader& ShaderLibrary::loadGraphicsProgram(const std::string& key,
                                                 const std::filesystem::path& vertexShaderPath,
                                                 const std::filesystem::path& fragmentShaderPath)
{
    const auto iterator = m_programs.find(key);
    if (iterator != m_programs.end())
    {
        return *iterator->second;
    }

    auto shader =
        std::make_unique<Shader>(shaderPath(vertexShaderPath), shaderPath(fragmentShaderPath));
    const Shader& shaderReference = *shader;
    m_programs.emplace(key, std::move(shader));
    return shaderReference;
}

void ShaderLibrary::unloadGraphicsProgram(const std::string& key) noexcept
{
    m_programs.erase(key);
}

bool ShaderLibrary::hasGraphicsProgram(const std::string& key) const noexcept
{
    return m_programs.find(key) != m_programs.end();
}

std::filesystem::path ShaderLibrary::shaderPath(const std::filesystem::path& shaderPath) const
{
    const std::filesystem::path requestedPath =
        shaderPath.is_absolute() ? shaderPath : (m_shaderDirectory / shaderPath);
    if (m_assetManager == nullptr)
    {
        throw std::runtime_error(
            "ShaderLibrary requires an AssetManager to resolve shader assets.");
    }

    const AssetHandle<ShaderAsset> handle = m_assetManager->findByPath<ShaderAsset>(requestedPath);
    if (!handle)
    {
        throw std::runtime_error("Shader dependency was not registered: '" +
                                 requestedPath.string() + "'.");
    }

    const std::shared_ptr<ShaderAsset> shaderAsset = m_assetManager->load(handle);
    if (shaderAsset == nullptr)
    {
        throw std::runtime_error("Failed to load shader asset: '" + requestedPath.string() + "'.");
    }

    std::ostringstream stream;
    stream << "Loaded ShaderAsset: " << shaderAsset->sourcePath().filename().string();
    Log::info("Assets", stream.str());
    return shaderAsset->sourcePath();
}

const std::filesystem::path& ShaderLibrary::shaderDirectory() const noexcept
{
    return m_shaderDirectory;
}
} // namespace engine
