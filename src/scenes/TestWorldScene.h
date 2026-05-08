#pragma once

#include "ecs/Entity.h"
#include "runtime/SceneAssetScope.h"
#include "primitives/Cone.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Plane.h"
#include "primitives/Pyramid.h"
#include "primitives/Sphere.h"
#include "runtime/SceneRuntime.h"
#include "systems/RenderSystem.h"
#include "world/Camera.h"
#include "world/Scene.h"
#include "world/TestWorld.h"

#include <memory>

namespace engine
{
class RenderPipeline;
class SceneMetasset;
class Shader;
class TextureAsset;
class Player;

class TestWorldScene final : public SceneRuntime
{
  public:
    explicit TestWorldScene(const SceneMetasset& sceneMetasset);
    ~TestWorldScene() override = default;

    const char* name() const override;
    void activate(AssetScope& assetScope) override;
    void deactivate(Renderer& renderer) override;

    Scene& scene() noexcept;
    const Scene& scene() const noexcept;
    AtmosphericWorldSettings& worldSettings() noexcept;
    const AtmosphericWorldSettings& worldSettings() const noexcept;
    AtmosphericRenderSettings& renderSettings() noexcept;
    const AtmosphericRenderSettings& renderSettings() const noexcept;
    AtmosphericRuntimeState& runtimeState() noexcept;
    const AtmosphericRuntimeState& runtimeState() const noexcept;
    void syncWorld();
    void ensureRuntimeEntities(ecs::Entity& playerEntity, ecs::Entity& debugCameraEntity);
    void syncRuntimeEntities(ecs::Entity playerEntity, const Player& player,
                             ecs::Entity debugCameraEntity, const Camera& debugCamera,
                             bool debugFreeCameraEnabled);
    void updateAtmosphere(float timeSeconds);
    void syncMoonVisual(const Vec3& activeCameraPosition);
    void renderWorld(Renderer& renderer, RenderPipeline& renderPipeline, int framebufferWidth,
                     int framebufferHeight, float timeSeconds, const Camera& activeCamera,
                     systems::FrameHistory& frameHistory);
    const std::shared_ptr<TextureAsset>& runtimeOverlayTexture() const noexcept;

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
