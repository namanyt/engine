#include "runtime/SceneAssetScope.h"

#include "core/ShaderLibrary.h"

#include <stdexcept>

namespace engine
{
SceneAssetScope::SceneAssetScope(std::shared_ptr<AssetManager> assetManager,
                                 ShaderLibrary& shaderLibrary,
                                 std::filesystem::path assetRootDirectory,
                                 std::filesystem::path shaderDirectory)
    : m_assetManager(std::move(assetManager)), m_shaderLibrary(shaderLibrary),
      m_assetRootDirectory(std::move(assetRootDirectory)),
      m_shaderDirectory(std::move(shaderDirectory))
{
}

void SceneAssetScope::bind(const SceneMetasset& sceneMetasset)
{
    clear();

    for (const SceneShaderProgramDependency& dependency : sceneMetasset.shaderProgramDependencies())
    {
        m_shaderDependencies.emplace(dependency.id, dependency);
    }

    for (const SceneAssetDependency& dependency : sceneMetasset.assetDependencies())
    {
        m_assetDependencies.emplace(dependency.id, dependency);
    }
}

void SceneAssetScope::clear()
{
    for (const auto& [dependencyId, shader] : m_loadedPrograms)
    {
        (void)dependencyId;
        (void)shader;
        const auto dependencyIterator = m_shaderDependencies.find(dependencyId);
        if (dependencyIterator != m_shaderDependencies.end())
        {
            m_shaderLibrary.unloadGraphicsProgram(dependencyIterator->second.programKey);
        }
    }

    m_loadedPrograms.clear();
    m_loadedAssets.clear();
    m_shaderDependencies.clear();
    m_assetDependencies.clear();
}

const Shader& SceneAssetScope::requireGraphicsProgram(const std::string& dependencyId)
{
    const auto loadedIterator = m_loadedPrograms.find(dependencyId);
    if (loadedIterator != m_loadedPrograms.end())
    {
        return *loadedIterator->second;
    }

    const SceneShaderProgramDependency& dependency = findShaderDependency(dependencyId);
    const Shader& shader = m_shaderLibrary.loadGraphicsProgram(
        dependency.programKey, dependency.vertexShaderPath, dependency.fragmentShaderPath);
    m_loadedPrograms.emplace(dependencyId, &shader);
    return shader;
}

const SceneShaderProgramDependency&
SceneAssetScope::findShaderDependency(const std::string& dependencyId) const
{
    const auto iterator = m_shaderDependencies.find(dependencyId);
    if (iterator == m_shaderDependencies.end())
    {
        throw std::runtime_error("Scene shader dependency was not declared: " + dependencyId);
    }

    return iterator->second;
}

const SceneAssetDependency&
SceneAssetScope::findAssetDependency(const std::string& dependencyId) const
{
    const auto iterator = m_assetDependencies.find(dependencyId);
    if (iterator == m_assetDependencies.end())
    {
        throw std::runtime_error("Scene asset dependency was not declared: " + dependencyId);
    }

    return iterator->second;
}

std::filesystem::path
SceneAssetScope::resolveAssetPath(const std::filesystem::path& assetPath) const
{
    if (assetPath.is_absolute())
    {
        return assetPath;
    }

    return (m_assetRootDirectory / assetPath).lexically_normal();
}
} // namespace engine
