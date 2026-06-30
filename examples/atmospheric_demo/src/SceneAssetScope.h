#pragma once

#include "assets/AssetHandle.h"
#include "assets/AssetManager.h"
#include "assets/Asset.h"

#include "SceneMetasset.h"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace engine
{
class Shader;
class ShaderLibrary;

class SceneAssetScope final
{
  public:
    SceneAssetScope(std::shared_ptr<AssetManager> assetManager, ShaderLibrary& shaderLibrary,
                    std::filesystem::path assetRootDirectory,
                    std::filesystem::path shaderDirectory);
    ~SceneAssetScope() = default;

    SceneAssetScope(const SceneAssetScope&) = delete;
    SceneAssetScope& operator=(const SceneAssetScope&) = delete;
    SceneAssetScope(SceneAssetScope&&) = delete;
    SceneAssetScope& operator=(SceneAssetScope&&) = delete;

    void bind(const SceneMetasset& sceneMetasset);
    void clear();

    const Shader& requireGraphicsProgram(const std::string& dependencyId);

    template <typename TAsset> std::shared_ptr<TAsset> requireAsset(const std::string& dependencyId)
    {
        const auto iterator = m_loadedAssets.find(dependencyId);
        if (iterator != m_loadedAssets.end())
        {
            return std::dynamic_pointer_cast<TAsset>(iterator->second);
        }

        const SceneAssetDependency& dependency = findAssetDependency(dependencyId);
        std::shared_ptr<TAsset> asset = loadAssetDependency<TAsset>(dependency);
        if (asset != nullptr)
        {
            m_loadedAssets.emplace(dependencyId, asset);
        }

        return asset;
    }

  private:
    const SceneShaderProgramDependency& findShaderDependency(const std::string& dependencyId) const;
    const SceneAssetDependency& findAssetDependency(const std::string& dependencyId) const;
    std::filesystem::path resolveAssetPath(const std::filesystem::path& assetPath) const;

    template <typename TAsset>
    std::shared_ptr<TAsset> loadAssetDependency(const SceneAssetDependency& dependency)
    {
        if (m_assetManager == nullptr)
        {
            if (dependency.required)
            {
                throw std::runtime_error("Scene asset scope is missing an AssetManager.");
            }

            return nullptr;
        }

        const std::filesystem::path resolvedPath = resolveAssetPath(dependency.assetPath);
        const auto handle = m_assetManager->template findByPath<TAsset>(resolvedPath);

        if (!handle)
        {
            if (dependency.required)
            {
                throw std::runtime_error("Missing required scene asset dependency: " +
                                         dependency.id + " at '" + resolvedPath.string() + "'.");
            }

            return nullptr;
        }

        std::shared_ptr<TAsset> asset = m_assetManager->template load<TAsset>(handle);
        if (asset == nullptr && dependency.required)
        {
            throw std::runtime_error("Failed to load required scene asset dependency: " +
                                     dependency.id + " at '" + resolvedPath.string() + "'.");
        }

        return asset;
    }

    std::shared_ptr<AssetManager> m_assetManager;
    ShaderLibrary& m_shaderLibrary;
    std::filesystem::path m_assetRootDirectory;
    std::filesystem::path m_shaderDirectory;
    std::unordered_map<std::string, SceneShaderProgramDependency> m_shaderDependencies;
    std::unordered_map<std::string, SceneAssetDependency> m_assetDependencies;
    std::unordered_map<std::string, const Shader*> m_loadedPrograms;
    std::unordered_map<std::string, std::shared_ptr<Asset>> m_loadedAssets;
};
} // namespace engine
