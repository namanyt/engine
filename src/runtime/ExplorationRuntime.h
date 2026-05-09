#pragma once

#include "ecs/Entity.h"
#include "runtime/ExplorationInput.h"
#include "runtime/RuntimeMode.h"
#include "runtime/StartupFlowOverlay.h"
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
    enum class OverlayView
    {
        None,
        Pause,
        Settings,
    };

    ExplorationInputState interpretInput(const RawInputState& inputState) const;
    void handleRuntimeInput(const UpdateContext& updateContext,
                            const ExplorationInputState& inputState);
    void handleOverlayInput(const UpdateContext& updateContext,
                            const ExplorationInputState& inputState);
    void applyRuntimeOverlay(Renderer& renderer) const;
    void refreshSettingsOverlay();
    const Camera& activeCamera() const;

    std::unique_ptr<SceneMetasset> m_sceneMetasset;
    std::unique_ptr<TestWorldScene> m_scene;
    StartupFlowOverlay m_entryFadeOverlay;
    StartupFlowOverlay m_pauseIdleOverlay;
    StartupFlowOverlay m_pauseResumeOverlay;
    StartupFlowOverlay m_pauseSettingsOverlay;
    StartupFlowOverlay m_pauseReturnOverlay;
    StartupFlowOverlay m_settingsOverlayTexture;
    std::shared_ptr<AssetManager> m_assetManager;
    SettingsOverlay m_settingsOverlay;
    Camera m_debugCamera;
    FreeCameraController m_debugCameraController;
    Player m_player;
    PlayerController m_playerController;
    systems::FrameHistory m_frameHistory{};
    ecs::Entity m_playerEntity = ecs::kInvalidEntity;
    ecs::Entity m_debugCameraEntity = ecs::kInvalidEntity;
    bool m_debugFreeCameraEnabled = false;
    OverlayView m_overlayView = OverlayView::None;
    PauseMenuSelection m_pauseSelection = PauseMenuSelection::None;
    float m_entryFadeElapsedSeconds = 0.0f;
};
} // namespace engine
