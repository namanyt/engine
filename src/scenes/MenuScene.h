#pragma once

#include "math/Types.h"
#include "runtime/SceneRuntime.h"

#include <memory>

namespace engine
{
class SceneAssetScope;
class SceneMetasset;
class TextureAsset;

class MenuScene final : public SceneRuntime
{
  public:
    explicit MenuScene(const SceneMetasset& sceneMetasset);
    ~MenuScene() override;

    const char* name() const override;
    void activate(AssetScope& assetScope) override;
    void deactivate(Renderer& renderer) override;

    const Color& clearColor() const noexcept;
    const std::shared_ptr<TextureAsset>& overlayTexture() const noexcept;

  private:
    std::unique_ptr<SceneAssetScope> m_sceneAssets;
    std::shared_ptr<TextureAsset> m_overlayTexture;
    Color m_clearColor{0.04f, 0.06f, 0.10f, 1.0f};
};
} // namespace engine
