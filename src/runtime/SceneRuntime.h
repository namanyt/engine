#pragma once

#include <filesystem>
#include <memory>

namespace engine
{
class AssetManager;
class SceneMetasset;
class Renderer;
class ShaderLibrary;

class SceneRuntime
{
  public:
    explicit SceneRuntime(const SceneMetasset& sceneMetasset);

    struct AssetScope final
    {
        std::shared_ptr<AssetManager> assetManager;
        ShaderLibrary& shaderLibrary;
        const std::filesystem::path& assetRootDirectory;
        const std::filesystem::path& shaderDirectory;
    };

    virtual ~SceneRuntime();

    const SceneMetasset& metasset() const noexcept;
    virtual const char* name() const = 0;
    virtual void activate(AssetScope& assetScope) = 0;
    virtual void deactivate(Renderer& renderer);

  private:
    const SceneMetasset& m_sceneMetasset;
};
} // namespace engine
