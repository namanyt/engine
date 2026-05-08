#include "runtime/SettingsOverlay.h"

#include "runtime/OverlayUiLayout.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace engine
{
namespace
{
struct ResolutionOption final
{
    int width = 1600;
    int height = 900;
};

constexpr std::array<ResolutionOption, 3> kResolutionOptions{{
    {1280, 720},
    {1600, 900},
    {1920, 1080},
}};

} // namespace

void SettingsOverlay::activate(const Application& application, bool pauseContext)
{
    m_pauseContext = pauseContext;
    m_hoverTarget = SettingsHoverTarget::None;
    m_draggingResolution = false;
    syncFromApplication(application);
    m_dirty = true;
}

SettingsOverlay::Result SettingsOverlay::update(const InputState& inputState,
                                                Application& application)
{
    Vec2 normalizedMouse{};
    const bool hasMouse = inputState.windowSize.x > 1.0f && inputState.windowSize.y > 1.0f;
    if (hasMouse)
    {
        normalizedMouse = overlayui::toDesignSpace(inputState.mousePosition, inputState.windowSize);
        updateHoverTarget(normalizedMouse);
    }
    else if (m_hoverTarget != SettingsHoverTarget::None)
    {
        m_hoverTarget = SettingsHoverTarget::None;
        m_dirty = true;
    }

    if (!inputState.mouseDown)
    {
        m_draggingResolution = false;
    }

    if (m_draggingResolution && hasMouse)
    {
        updateResolutionFromMouse(normalizedMouse);
    }

    if (inputState.cancel)
    {
        return Result::Close;
    }

    if (!inputState.click)
    {
        return Result::None;
    }

    switch (m_hoverTarget)
    {
    case SettingsHoverTarget::ResolutionSlider:
        m_draggingResolution = true;
        if (hasMouse)
        {
            updateResolutionFromMouse(normalizedMouse);
        }
        return Result::None;
    case SettingsHoverTarget::WindowedMode:
        if (m_windowMode != Application::WindowMode::Windowed)
        {
            m_windowMode = Application::WindowMode::Windowed;
            m_dirty = true;
        }
        return Result::None;
    case SettingsHoverTarget::BorderlessMode:
        if (m_windowMode != Application::WindowMode::BorderlessFullscreen)
        {
            m_windowMode = Application::WindowMode::BorderlessFullscreen;
            m_dirty = true;
        }
        return Result::None;
    case SettingsHoverTarget::ExclusiveMode:
        if (m_windowMode != Application::WindowMode::ExclusiveFullscreen)
        {
            m_windowMode = Application::WindowMode::ExclusiveFullscreen;
            m_dirty = true;
        }
        return Result::None;
    case SettingsHoverTarget::VSyncToggle:
        m_vsyncEnabled = !m_vsyncEnabled;
        m_dirty = true;
        return Result::None;
    case SettingsHoverTarget::ApplyButton:
        if (hasPendingChanges())
        {
            applyPendingSettings(application);
        }
        return Result::None;
    case SettingsHoverTarget::BackButton:
        return Result::Close;
    case SettingsHoverTarget::None:
    default:
        return Result::None;
    }
}

bool SettingsOverlay::consumeDirty()
{
    const bool dirty = m_dirty;
    m_dirty = false;
    return dirty;
}

SettingsOverlayViewModel SettingsOverlay::viewModel() const noexcept
{
    SettingsOverlayViewModel viewModel{};
    viewModel.hoverTarget = m_hoverTarget;
    viewModel.resolutionIndex = m_resolutionIndex;
    viewModel.resolutionCount = static_cast<int>(kResolutionOptions.size());
    viewModel.resolutionWidth =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)].width;
    viewModel.resolutionHeight =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)].height;
    viewModel.windowMode = m_windowMode;
    viewModel.vSyncEnabled = m_vsyncEnabled;
    viewModel.applyEnabled = hasPendingChanges();
    viewModel.pauseContext = m_pauseContext;
    viewModel.resolutionDragging = m_draggingResolution;
    return viewModel;
}

void SettingsOverlay::syncFromApplication(const Application& application)
{
    m_appliedSettings = application.displaySettings();
    m_resolutionIndex = resolutionIndexFor(m_appliedSettings);
    m_windowMode = application.windowMode();
    m_vsyncEnabled = application.isVSyncEnabled();
}

int SettingsOverlay::resolutionIndexFor(const Application::DisplaySettings& settings) const noexcept
{
    for (int index = 0; index < static_cast<int>(kResolutionOptions.size()); ++index)
    {
        const ResolutionOption& option = kResolutionOptions[static_cast<std::size_t>(index)];
        if (option.width == settings.width && option.height == settings.height)
        {
            return index;
        }
    }

    return 1;
}

void SettingsOverlay::updateHoverTarget(const Vec2& normalizedMouse)
{
    SettingsHoverTarget hoveredTarget = SettingsHoverTarget::None;
    if (overlayui::contains(overlayui::kSettingsResolutionSliderRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::ResolutionSlider;
    }
    else if (overlayui::contains(overlayui::kSettingsWindowedModeRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::WindowedMode;
    }
    else if (overlayui::contains(overlayui::kSettingsBorderlessModeRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::BorderlessMode;
    }
    else if (overlayui::contains(overlayui::kSettingsExclusiveModeRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::ExclusiveMode;
    }
    else if (overlayui::contains(overlayui::kSettingsVSyncRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::VSyncToggle;
    }
    else if (overlayui::contains(overlayui::kSettingsApplyRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::ApplyButton;
    }
    else if (overlayui::contains(overlayui::kSettingsBackRect, normalizedMouse))
    {
        hoveredTarget = SettingsHoverTarget::BackButton;
    }

    if (m_hoverTarget != hoveredTarget)
    {
        m_hoverTarget = hoveredTarget;
        m_dirty = true;
    }
}

void SettingsOverlay::updateResolutionFromMouse(const Vec2& normalizedMouse)
{
    const float sliderWidth = overlayui::kSettingsResolutionSliderRect.right -
                              overlayui::kSettingsResolutionSliderRect.left;
    if (sliderWidth <= 0.0f)
    {
        return;
    }

    const float t =
        overlayui::normalizedT(normalizedMouse.x, overlayui::kSettingsResolutionSliderRect.left,
                               overlayui::kSettingsResolutionSliderRect.right);
    const int newIndex = static_cast<int>(
        std::round(t * static_cast<float>(static_cast<int>(kResolutionOptions.size()) - 1)));
    if (newIndex != m_resolutionIndex)
    {
        m_resolutionIndex = newIndex;
        m_dirty = true;
    }
}

void SettingsOverlay::applyPendingSettings(Application& application)
{
    const ResolutionOption& resolution =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)];
    application.setWindowResolution(resolution.width, resolution.height);
    application.setWindowMode(m_windowMode);
    application.setVSyncEnabled(m_vsyncEnabled);
    syncFromApplication(application);
    m_dirty = true;
}

bool SettingsOverlay::hasPendingChanges() const noexcept
{
    const ResolutionOption& resolution =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)];
    return resolution.width != m_appliedSettings.width ||
           resolution.height != m_appliedSettings.height ||
           m_windowMode != m_appliedSettings.windowMode ||
           m_vsyncEnabled != m_appliedSettings.vSyncEnabled;
}
} // namespace engine
