#pragma once

#include "math/Types.h"

#include "SceneRuntime.h"

#include <memory>

namespace engine
{
class SceneAssetScope;
class SceneMetasset;
class TextureAsset;

class LoadingScene final : public SceneRuntime
{
  public:
    explicit LoadingScene(const SceneMetasset& sceneMetasset);
    ~LoadingScene() override;

    const char* name() const override;
    void activate(AssetScope& assetScope) override;
    void deactivate(Renderer& renderer) override;

    const Color& clearColor() const noexcept;
    const std::shared_ptr<TextureAsset>& overlayTexture() const noexcept;

  private:
    std::unique_ptr<SceneAssetScope> m_sceneAssets;
    std::shared_ptr<TextureAsset> m_overlayTexture;
    Color m_clearColor{0.02f, 0.03f, 0.05f, 1.0f};
};
} // namespace engine
