#pragma once

#include "AtmosphericSceneRuntime.h"

#include "SceneAssetScope.h"

#include "primitives/Cone.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Plane.h"
#include "primitives/Pyramid.h"
#include "primitives/Sphere.h"

#include <memory>

namespace engine
{
class RenderPipeline;
class SceneMetasset;
class Shader;
class TextureAsset;
class Player;

class DaylightSandboxScene final : public AtmosphericSceneRuntime
{
  public:
    explicit DaylightSandboxScene(const SceneMetasset& sceneMetasset);
    ~DaylightSandboxScene() override = default;

    const char* name() const override;
    void activate(AssetScope& assetScope) override;
    void deactivate(Renderer& renderer) override;

    Scene& scene() noexcept override;
    const Scene& scene() const noexcept override;
    AtmosphericWorldSettings& worldSettings() noexcept override;
    const AtmosphericWorldSettings& worldSettings() const noexcept override;
    AtmosphericRenderSettings& renderSettings() noexcept override;
    const AtmosphericRenderSettings& renderSettings() const noexcept override;
    AtmosphericRuntimeState& runtimeState() noexcept override;
    const AtmosphericRuntimeState& runtimeState() const noexcept override;
    Vec3 defaultPlayerSpawn() const noexcept override;
    void syncWorld() override;
    void ensureRuntimeEntities(ecs::Entity& playerEntity, ecs::Entity& debugCameraEntity) override;
    void syncRuntimeEntities(ecs::Entity playerEntity, const Player& player,
                             ecs::Entity debugCameraEntity, const Camera& debugCamera,
                             bool debugFreeCameraEnabled) override;
    void updateAtmosphere(float timeSeconds) override;
    void syncMoonVisual(const Vec3& activeCameraPosition) override;
    void renderWorld(Renderer& renderer, RenderPipeline& renderPipeline, int framebufferWidth,
                     int framebufferHeight, float timeSeconds, const Camera& activeCamera,
                     systems::FrameHistory& frameHistory,
                     const std::vector<systems::RenderItem>& extraRenderItems) override;
    const std::shared_ptr<TextureAsset>& runtimeOverlayTexture() const noexcept override;

  private:
    Scene m_scene;
    AtmosphericWorldSettings m_worldSettings{};
    AtmosphericRenderSettings m_renderSettings{};
    AtmosphericRuntimeState m_runtimeState{};
    std::unique_ptr<SceneAssetScope> m_sceneAssets;
    const Shader* m_surfaceShader = nullptr;
    std::shared_ptr<TextureAsset> m_runtimeOverlayTexture;
    TestWorldAssets m_worldAssets{};
    Plane m_plane;
    Cube m_cube;
    Pyramid m_pyramid;
    Sphere m_sphere;
    Cylinder m_cylinder;
    Cone m_cone;
};
} // namespace engine
