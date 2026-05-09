#pragma once

#include "math/Types.h"
#include "runtime/RuntimeMode.h"
#include "runtime/SettingsOverlay.h"
#include "runtime/StartupFlowOverlay.h"

#include <memory>

namespace engine
{
class MenuRuntime final : public RuntimeMode
{
  public:
    MenuRuntime();
    ~MenuRuntime() override;

    const char* name() const override;
    void activate(ActivationContext& activationContext) override;
    void deactivate(Renderer& renderer) override;
    void update(const UpdateContext& updateContext) override;
    void render(const RenderContext& renderContext) override;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    void drawDebugUi(const DebugUiContext& debugUiContext) override;
#endif

  private:
    enum class View
    {
        Main,
        Settings,
    };

    enum class Phase
    {
        Browsing,
        FadingOut,
    };

    enum class Selection
    {
        None,
        StartExploration,
        Settings,
        Quit,
    };

    struct MenuInputState final
    {
        bool cancel = false;
        bool click = false;
        bool hoverStart = false;
        bool hoverSettings = false;
        bool hoverQuit = false;
    };

    MenuInputState interpretInput(const RawInputState& inputState) const;
    Color activeClearColor(float timeSeconds) const;
    void applyOverlayTexture(Renderer& renderer) const;
    void refreshSettingsOverlay();
    float fadeProgress() const;

    StartupFlowOverlay m_startSelectedOverlay;
    StartupFlowOverlay m_settingsSelectedOverlay;
    StartupFlowOverlay m_quitSelectedOverlay;
    StartupFlowOverlay m_idleOverlay;
    StartupFlowOverlay m_settingsOverlayTexture;
    std::shared_ptr<AssetManager> m_assetManager;
    SettingsOverlay m_settingsOverlay;
    View m_view = View::Main;
    Phase m_phase = Phase::Browsing;
    Selection m_selectedAction = Selection::None;
    float m_phaseElapsedSeconds = 0.0f;
};
} // namespace engine
