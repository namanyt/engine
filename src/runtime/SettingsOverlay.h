#pragma once

#include "Application.h"

namespace engine
{
enum class SettingsHoverTarget
{
    None,
    ResolutionSlider,
    WindowedMode,
    BorderlessMode,
    ExclusiveMode,
    VSyncToggle,
    ApplyButton,
    BackButton,
};

struct SettingsOverlayViewModel final
{
    SettingsHoverTarget hoverTarget = SettingsHoverTarget::None;
    int resolutionIndex = 0;
    int resolutionCount = 1;
    int resolutionWidth = 1600;
    int resolutionHeight = 900;
    Application::WindowMode windowMode = Application::WindowMode::Windowed;
    bool vSyncEnabled = false;
    bool applyEnabled = false;
    bool pauseContext = false;
    bool resolutionDragging = false;
};

class SettingsOverlay final
{
  public:
    struct InputState final
    {
        bool cancel = false;
        bool click = false;
        bool mouseDown = false;
        Vec2 mousePosition{};
        Vec2 windowSize{};
    };

    enum class Result
    {
        None,
        Close,
    };

    void activate(const Application& application, bool pauseContext);
    Result update(const InputState& inputState, Application& application);
    bool consumeDirty();
    SettingsOverlayViewModel viewModel() const noexcept;

  private:
    void syncFromApplication(const Application& application);
    int resolutionIndexFor(const Application::DisplaySettings& settings) const noexcept;
    void updateHoverTarget(const Vec2& normalizedMouse);
    void updateResolutionFromMouse(const Vec2& normalizedMouse);
    void applyPendingSettings(Application& application);
    bool hasPendingChanges() const noexcept;

    SettingsHoverTarget m_hoverTarget = SettingsHoverTarget::None;
    int m_resolutionIndex = 1;
    Application::DisplaySettings m_appliedSettings{};
    Application::WindowMode m_windowMode = Application::WindowMode::Windowed;
    bool m_vsyncEnabled = false;
    bool m_pauseContext = false;
    bool m_draggingResolution = false;
    bool m_dirty = true;
};
} // namespace engine
