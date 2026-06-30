#include "MenuScene.h"

#include "assets/TextureAsset.h"
#include "core/Renderer.h"

#include "MenuScene.metasset.h"
#include "SceneAssetScope.h"

namespace engine
{
MenuScene::MenuScene(const SceneMetasset& sceneMetasset) : SceneRuntime(sceneMetasset) {}

MenuScene::~MenuScene() = default;

const char* MenuScene::name() const
{
    return "MenuScene";
}

void MenuScene::activate(AssetScope& assetScope)
{
    m_sceneAssets = std::make_unique<SceneAssetScope>(
        assetScope.assetManager, assetScope.shaderLibrary, assetScope.assetRootDirectory,
        assetScope.shaderDirectory);
    m_sceneAssets->bind(metasset());
    m_overlayTexture =
        m_sceneAssets->requireAsset<TextureAsset>(MenuSceneMetasset::kOverlayTextureId);
}

void MenuScene::deactivate(Renderer& renderer)
{
    renderer.clearRuntimeOverlayTexture();
    m_overlayTexture.reset();

    if (m_sceneAssets != nullptr)
    {
        m_sceneAssets->clear();
        m_sceneAssets.reset();
    }
}

const Color& MenuScene::clearColor() const noexcept
{
    return m_clearColor;
}

const std::shared_ptr<TextureAsset>& MenuScene::overlayTexture() const noexcept
{
    return m_overlayTexture;
}
} // namespace engine
