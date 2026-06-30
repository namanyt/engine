#include "LoadingScene.h"

#include "assets/TextureAsset.h"
#include "core/Renderer.h"

#include "LoadingScene.metasset.h"
#include "SceneAssetScope.h"

namespace engine
{
LoadingScene::LoadingScene(const SceneMetasset& sceneMetasset) : SceneRuntime(sceneMetasset) {}

LoadingScene::~LoadingScene() = default;

const char* LoadingScene::name() const
{
    return "LoadingScene";
}

void LoadingScene::activate(AssetScope& assetScope)
{
    m_sceneAssets = std::make_unique<SceneAssetScope>(
        assetScope.assetManager, assetScope.shaderLibrary, assetScope.assetRootDirectory,
        assetScope.shaderDirectory);
    m_sceneAssets->bind(metasset());
    m_overlayTexture =
        m_sceneAssets->requireAsset<TextureAsset>(LoadingSceneMetasset::kOverlayTextureId);
}

void LoadingScene::deactivate(Renderer& renderer)
{
    renderer.clearRuntimeOverlayTexture();
    m_overlayTexture.reset();

    if (m_sceneAssets != nullptr)
    {
        m_sceneAssets->clear();
        m_sceneAssets.reset();
    }
}

const Color& LoadingScene::clearColor() const noexcept
{
    return m_clearColor;
}

const std::shared_ptr<TextureAsset>& LoadingScene::overlayTexture() const noexcept
{
    return m_overlayTexture;
}
} // namespace engine
