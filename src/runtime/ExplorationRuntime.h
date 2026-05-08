#pragma once

#include "ecs/Entity.h"
#include "runtime/ExplorationInput.h"
#include "runtime/RuntimeMode.h"
#include "systems/RenderSystem.h"
#include "world/Camera.h"
#include "world/FreeCameraController.h"
#include "world/Player.h"
#include "world/PlayerController.h"

#include <memory>

namespace engine
{
class SceneMetasset;
class TestWorldScene;

class ExplorationRuntime final : public RuntimeMode
{
  public:
    ExplorationRuntime();
    ~ExplorationRuntime() override;

    const char* name() const override;
    void prepareActivation(ActivationContext& activationContext) override;
    void activate(ActivationContext& activationContext) override;
    void deactivate(Renderer& renderer) override;
    void update(const UpdateContext& updateContext) override;
    void render(const RenderContext& renderContext) override;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    void drawDebugUi(const DebugUiContext& debugUiContext) override;
#endif

  private:
    ExplorationInputState interpretInput(const RawInputState& inputState) const;
    void handleRuntimeInput(const UpdateContext& updateContext,
                            const ExplorationInputState& inputState);
    const Camera& activeCamera() const;

    std::unique_ptr<SceneMetasset> m_sceneMetasset;
    std::unique_ptr<TestWorldScene> m_scene;
    Camera m_debugCamera;
    FreeCameraController m_debugCameraController;
    Player m_player;
    PlayerController m_playerController;
    systems::FrameHistory m_frameHistory{};
    ecs::Entity m_playerEntity = ecs::kInvalidEntity;
    ecs::Entity m_debugCameraEntity = ecs::kInvalidEntity;
    bool m_debugFreeCameraEnabled = false;
};
} // namespace engine
