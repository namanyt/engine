#pragma once

#include "runtime/SettingsPage.h"

namespace engine
{
enum class SettingsOverlayDirtyRegion : unsigned int
{
    None = 0u,
    Base = 1u << 0u,
    Content = 1u << 1u,
};

SettingsOverlayDirtyRegion operator|(SettingsOverlayDirtyRegion left,
                                     SettingsOverlayDirtyRegion right) noexcept;
SettingsOverlayDirtyRegion& operator|=(SettingsOverlayDirtyRegion& left,
                                       SettingsOverlayDirtyRegion right) noexcept;
bool hasDirtyRegion(SettingsOverlayDirtyRegion value, SettingsOverlayDirtyRegion flag) noexcept;

class SettingsOverlay final
{
  public:
    struct InputState final
    {
        bool cancel = false;
        bool click = false;
        bool mouseDown = false;
        float mouseScrollDelta = 0.0f;
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
    SettingsOverlayDirtyRegion consumeDirtyRegions();
    SettingsOverlayViewModel viewModel() const noexcept;

  private:
    void syncFromApplication(const Application& application);
    int resolutionIndexFor(const Application::DisplaySettings& settings) const noexcept;
    void scrollContent(float scrollDelta, const Vec2& normalizedMouse) noexcept;
    void updateScrollFromMouse(const Vec2& normalizedMouse, bool centerOnCursor) noexcept;
    void updateHoverTarget(const Vec2& normalizedMouse);
    void updateSliderFromMouse(SettingsHoverTargetType sliderType, const Vec2& normalizedMouse,
                               Application& application);
    void applyPendingSettings(Application& application);
    bool hasPendingChanges() const noexcept;
    void markDirty(SettingsOverlayDirtyRegion dirtyRegion) noexcept;

    SettingsHoverTarget m_hoverTarget{};
    SettingsHoverTargetType m_draggingSlider = SettingsHoverTargetType::None;
    SettingsCategory m_activeCategory = SettingsCategory::Display;
    int m_resolutionIndex = 1;
    Application::DisplaySettings m_appliedSettings{};
    float m_appliedMasterVolume = 1.0f;
    float m_appliedMusicVolume = 1.0f;
    float m_appliedUiVolume = 1.0f;
    float m_appliedVnVolume = 1.0f;
    float m_appliedAmbientVolume = 1.0f;
    float m_appliedSfxVolume = 1.0f;
    float m_appliedMouseSensitivity = 0.10f;
    float m_appliedTypingCharactersPerSecond = 30.0f;
    bool m_appliedAutoAdvanceEnabled = false;
    Application::WindowMode m_windowMode = Application::WindowMode::Windowed;
    bool m_vsyncEnabled = false;
    bool m_autoAdvanceEnabled = false;
    bool m_pauseContext = false;
    float m_masterVolume = 0.85f;
    float m_musicVolume = 0.70f;
    float m_uiVolume = 1.0f;
    float m_vnVolume = 1.0f;
    float m_ambientVolume = 1.0f;
    float m_sfxVolume = 1.0f;
    float m_mouseSensitivity = 0.52f;
    float m_typingSpeed = 0.45f;
    float m_contentScrollOffset = 0.0f;
    float m_scrollThumbGrabOffset = 0.0f;
    bool m_draggingScrollThumb = false;
    SettingsOverlayDirtyRegion m_dirtyRegions =
        SettingsOverlayDirtyRegion::Base | SettingsOverlayDirtyRegion::Content;
};
} // namespace engine
