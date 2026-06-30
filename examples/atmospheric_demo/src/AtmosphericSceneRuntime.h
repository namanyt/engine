#pragma once

#include "SceneRuntime.h"
#include "systems/RenderSystem.h"
#include "world/Camera.h"
#include "world/TestWorld.h"

#include <memory>

namespace engine
{
class Player;
class RenderPipeline;
class Renderer;
class TextureAsset;

class AtmosphericSceneRuntime : public SceneRuntime
{
  public:
    using SceneRuntime::SceneRuntime;
    ~AtmosphericSceneRuntime() override = default;

    virtual Scene& scene() noexcept = 0;
    virtual const Scene& scene() const noexcept = 0;
    virtual AtmosphericWorldSettings& worldSettings() noexcept = 0;
    virtual const AtmosphericWorldSettings& worldSettings() const noexcept = 0;
    virtual AtmosphericRenderSettings& renderSettings() noexcept = 0;
    virtual const AtmosphericRenderSettings& renderSettings() const noexcept = 0;
    virtual AtmosphericRuntimeState& runtimeState() noexcept = 0;
    virtual const AtmosphericRuntimeState& runtimeState() const noexcept = 0;
    virtual Vec3 defaultPlayerSpawn() const noexcept = 0;
    virtual void syncWorld() = 0;
    virtual void ensureRuntimeEntities(ecs::Entity& playerEntity,
                                       ecs::Entity& debugCameraEntity) = 0;
    virtual void syncRuntimeEntities(ecs::Entity playerEntity, const Player& player,
                                     ecs::Entity debugCameraEntity, const Camera& debugCamera,
                                     bool debugFreeCameraEnabled) = 0;
    virtual void updateAtmosphere(float timeSeconds) = 0;
    virtual void syncMoonVisual(const Vec3& activeCameraPosition) = 0;
    virtual void renderWorld(Renderer& renderer, RenderPipeline& renderPipeline,
                             int framebufferWidth, int framebufferHeight, float timeSeconds,
                             const Camera& activeCamera, systems::FrameHistory& frameHistory,
                             const std::vector<systems::RenderItem>& extraRenderItems) = 0;
    virtual const std::shared_ptr<TextureAsset>& runtimeOverlayTexture() const noexcept = 0;
};
} // namespace engine
