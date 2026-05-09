#pragma once

#include "ecs/Entity.h"
#include "runtime/ExplorationInput.h"
#include "runtime/InteractionPromptRenderable.h"
#include "runtime/RuntimeIds.h"
#include "runtime/RuntimeMode.h"
#include "runtime/StartupFlowOverlay.h"
#include "systems/InteractionSystem.h"
#include "systems/RenderSystem.h"
#include "world/Camera.h"
#include "world/FreeCameraController.h"
#include "world/Player.h"
#include "world/PlayerController.h"

#include <memory>

namespace engine
{
class SceneMetasset;
class AtmosphericSceneRuntime;
class TextureAsset;

class ExplorationRuntime final : public RuntimeMode
{
  public:
    explicit ExplorationRuntime(RuntimeId runtimeId = RuntimeId::FoggyTestWorld);
    ~ExplorationRuntime() override;

    const char* name() const override;
    void prepareActivation(ActivationContext& activationContext) override;
    void activate(ActivationContext& activationContext) override;
    void deactivate(Renderer& renderer) override;
    void update(const UpdateContext& updateContext) override;
    void render(const RenderContext& renderContext) override;
    bool canRenderLoadingPreview() const override;
    void renderLoadingPreview(const RenderContext& renderContext) override;

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
    void updateInteractionState(const ExplorationInputState& inputState, float deltaSeconds);
    void appendInteractionPromptRenderItem(Renderer& renderer,
                                           std::vector<systems::RenderItem>& extraRenderItems);
    void applyRuntimeOverlay(Renderer& renderer) const;
    void refreshSettingsOverlay();
    const Camera& activeCamera() const;
    void setDebugFreeCameraEnabled(bool previousEnabled, bool enabled);
    void requestWorldLoad(RuntimeId targetRuntimeId);
    void refreshInteractionDebugState();

    RuntimeId m_runtimeId = RuntimeId::FoggyTestWorld;
    std::unique_ptr<SceneMetasset> m_sceneMetasset;
    std::unique_ptr<AtmosphericSceneRuntime> m_scene;
    StartupFlowOverlay m_entryFadeOverlay;
    StartupFlowOverlay m_pauseIdleOverlay;
    StartupFlowOverlay m_pauseResumeOverlay;
    StartupFlowOverlay m_pauseSettingsOverlay;
    StartupFlowOverlay m_pauseReturnOverlay;
    StartupFlowOverlay m_settingsOverlayTexture;
    std::shared_ptr<AssetManager> m_assetManager;
    std::shared_ptr<TextureAsset> m_alternatePanelTexture;
    std::shared_ptr<TextureAsset> m_panelOverrideTexture;
    SettingsOverlay m_settingsOverlay;
    Camera m_debugCamera;
    FreeCameraController m_debugCameraController;
    InteractionPromptRenderable m_interactionPromptRenderable;
    Player m_player;
    PlayerController m_playerController;
    systems::InteractionState m_interactionState{};
    systems::FrameHistory m_frameHistory{};
    systems::FrameHistory m_loadingPreviewFrameHistory{};
    ecs::Entity m_playerEntity = ecs::kInvalidEntity;
    ecs::Entity m_debugCameraEntity = ecs::kInvalidEntity;
    bool m_debugFreeCameraEnabled = false;
    OverlayView m_overlayView = OverlayView::None;
    PauseMenuSelection m_pauseSelection = PauseMenuSelection::None;
    float m_entryFadeElapsedSeconds = 0.0f;
    float m_interactionPromptOpacity = 0.0f;
    Vec3 m_interactionPromptWorldPosition{};
    std::string m_activeInteractionPrompt;
    std::string m_activeInteractionId;
};
} // namespace engine
