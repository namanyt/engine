#include "runtime/SettingsOverlay.h"

#include "runtime/SettingsPage.h"

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

struct SettingsValueMapping final
{
    static float normalizeTypingCharactersPerSecond(float charactersPerSecond) noexcept
    {
        return std::clamp((charactersPerSecond - 12.0f) / 36.0f, 0.0f, 1.0f);
    }

    static float denormalizeTypingCharactersPerSecond(float normalizedValue) noexcept
    {
        return 12.0f + std::clamp(normalizedValue, 0.0f, 1.0f) * 36.0f;
    }

    static float normalizeMouseSensitivity(float sensitivity) noexcept
    {
        return std::clamp((sensitivity - 0.03f) / 0.15f, 0.0f, 1.0f);
    }

    static float denormalizeMouseSensitivity(float normalizedValue) noexcept
    {
        return 0.03f + std::clamp(normalizedValue, 0.0f, 1.0f) * 0.15f;
    }
};

SettingsOverlayDirtyRegion
dirtyRegionForHoverTarget(const SettingsHoverTarget& hoverTarget) noexcept
{
    if (hoverTarget.type == SettingsHoverTargetType::Category)
    {
        return SettingsOverlayDirtyRegion::Base;
    }

    if (hoverTarget.type == SettingsHoverTargetType::None)
    {
        return SettingsOverlayDirtyRegion::None;
    }

    return SettingsOverlayDirtyRegion::Content;
}

} // namespace

SettingsOverlayDirtyRegion operator|(SettingsOverlayDirtyRegion left,
                                     SettingsOverlayDirtyRegion right) noexcept
{
    return static_cast<SettingsOverlayDirtyRegion>(static_cast<unsigned int>(left) |
                                                   static_cast<unsigned int>(right));
}

SettingsOverlayDirtyRegion& operator|=(SettingsOverlayDirtyRegion& left,
                                       SettingsOverlayDirtyRegion right) noexcept
{
    left = left | right;
    return left;
}

bool hasDirtyRegion(SettingsOverlayDirtyRegion value, SettingsOverlayDirtyRegion flag) noexcept
{
    return (static_cast<unsigned int>(value) & static_cast<unsigned int>(flag)) != 0u;
}

void SettingsOverlay::activate(const Application& application, bool pauseContext)
{
    m_pauseContext = pauseContext;
    m_hoverTarget = {};
    m_draggingSlider = SettingsHoverTargetType::None;
    syncFromApplication(application);
    m_dirtyRegions = SettingsOverlayDirtyRegion::Base | SettingsOverlayDirtyRegion::Content;
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
        if (inputState.mouseDown && m_draggingSlider != SettingsHoverTargetType::None)
        {
            updateSliderFromMouse(m_draggingSlider, normalizedMouse);
            const SettingsHoverTarget draggingHoverTarget{m_draggingSlider, m_activeCategory};
            if (m_hoverTarget != draggingHoverTarget)
            {
                m_hoverTarget = draggingHoverTarget;
                markDirty(SettingsOverlayDirtyRegion::Content);
            }
        }
    }
    else if (m_hoverTarget.type != SettingsHoverTargetType::None)
    {
        markDirty(dirtyRegionForHoverTarget(m_hoverTarget));
        m_hoverTarget = {};
    }

    if (!inputState.mouseDown)
    {
        m_draggingSlider = SettingsHoverTargetType::None;
    }

    if (inputState.cancel)
    {
        return Result::Close;
    }

    if (!inputState.click)
    {
        return Result::None;
    }

    switch (m_hoverTarget.type)
    {
    case SettingsHoverTargetType::Category:
        if (m_activeCategory != m_hoverTarget.category)
        {
            m_activeCategory = m_hoverTarget.category;
            m_draggingSlider = SettingsHoverTargetType::None;
            markDirty(SettingsOverlayDirtyRegion::Base | SettingsOverlayDirtyRegion::Content);
        }
        return Result::None;
    case SettingsHoverTargetType::ResolutionSlider:
    case SettingsHoverTargetType::MasterVolumeSlider:
    case SettingsHoverTargetType::MusicVolumeSlider:
    case SettingsHoverTargetType::MouseSensitivitySlider:
    case SettingsHoverTargetType::TypingSpeedSlider:
        m_draggingSlider = m_hoverTarget.type;
        if (hasMouse)
        {
            updateSliderFromMouse(m_hoverTarget.type, normalizedMouse);
        }
        return Result::None;
    case SettingsHoverTargetType::WindowedMode:
        if (m_windowMode != Application::WindowMode::Windowed)
        {
            m_windowMode = Application::WindowMode::Windowed;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        return Result::None;
    case SettingsHoverTargetType::BorderlessMode:
        if (m_windowMode != Application::WindowMode::BorderlessFullscreen)
        {
            m_windowMode = Application::WindowMode::BorderlessFullscreen;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        return Result::None;
    case SettingsHoverTargetType::ExclusiveMode:
        if (m_windowMode != Application::WindowMode::ExclusiveFullscreen)
        {
            m_windowMode = Application::WindowMode::ExclusiveFullscreen;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        return Result::None;
    case SettingsHoverTargetType::VSyncToggle:
        m_vsyncEnabled = !m_vsyncEnabled;
        markDirty(SettingsOverlayDirtyRegion::Content);
        return Result::None;
    case SettingsHoverTargetType::AutoAdvanceToggle:
        m_autoAdvanceEnabled = !m_autoAdvanceEnabled;
        markDirty(SettingsOverlayDirtyRegion::Content);
        return Result::None;
    case SettingsHoverTargetType::ApplyButton:
        if (hasPendingChanges())
        {
            applyPendingSettings(application);
        }
        return Result::None;
    case SettingsHoverTargetType::BackButton:
        return Result::Close;
    case SettingsHoverTargetType::None:
    default:
        return Result::None;
    }
}

SettingsOverlayDirtyRegion SettingsOverlay::consumeDirtyRegions()
{
    const SettingsOverlayDirtyRegion dirtyRegions = m_dirtyRegions;
    m_dirtyRegions = SettingsOverlayDirtyRegion::None;
    return dirtyRegions;
}

SettingsOverlayViewModel SettingsOverlay::viewModel() const noexcept
{
    SettingsOverlayViewModel viewModel{};
    viewModel.hoverTarget = m_hoverTarget;
    viewModel.draggingSlider = m_draggingSlider;
    viewModel.activeCategory = m_activeCategory;
    viewModel.resolutionIndex = m_resolutionIndex;
    viewModel.resolutionCount = static_cast<int>(kResolutionOptions.size());
    viewModel.resolutionWidth =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)].width;
    viewModel.resolutionHeight =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)].height;
    viewModel.windowMode = m_windowMode;
    viewModel.vSyncEnabled = m_vsyncEnabled;
    viewModel.autoAdvanceEnabled = m_autoAdvanceEnabled;
    viewModel.applyEnabled = hasPendingChanges();
    viewModel.pauseContext = m_pauseContext;
    viewModel.resolutionDragging = m_draggingSlider == SettingsHoverTargetType::ResolutionSlider;
    viewModel.masterVolume = m_masterVolume;
    viewModel.musicVolume = m_musicVolume;
    viewModel.mouseSensitivity = m_mouseSensitivity;
    viewModel.typingSpeed = m_typingSpeed;
    viewModel.contentScrollOffset = m_contentScrollOffset;
    return viewModel;
}

void SettingsOverlay::syncFromApplication(const Application& application)
{
    m_appliedSettings = application.displaySettings();
    m_resolutionIndex = resolutionIndexFor(m_appliedSettings);
    m_windowMode = application.windowMode();
    m_vsyncEnabled = application.isVSyncEnabled();
    m_masterVolume = application.audioSettings().masterVolume;
    m_musicVolume = application.audioSettings().musicVolume;
    m_mouseSensitivity = SettingsValueMapping::normalizeMouseSensitivity(
        application.inputSettings().mouseSensitivity);
    m_typingSpeed = SettingsValueMapping::normalizeTypingCharactersPerSecond(
        application.vnSettings().typingCharactersPerSecond);
    m_autoAdvanceEnabled = application.vnSettings().autoAdvanceEnabled;
    m_appliedMasterVolume = application.audioSettings().masterVolume;
    m_appliedMusicVolume = application.audioSettings().musicVolume;
    m_appliedMouseSensitivity = application.inputSettings().mouseSensitivity;
    m_appliedTypingCharactersPerSecond = application.vnSettings().typingCharactersPerSecond;
    m_appliedAutoAdvanceEnabled = application.vnSettings().autoAdvanceEnabled;
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
    const SettingsPageModel page = buildSettingsPageModel(viewModel());
    const SettingsHoverTarget previousHoverTarget = m_hoverTarget;
    SettingsHoverTarget hoveredTarget{};

    if (overlayui::contains(page.chrome.backButtonBounds, normalizedMouse))
    {
        hoveredTarget.type = SettingsHoverTargetType::BackButton;
    }
    else if (overlayui::contains(page.chrome.applyButtonBounds, normalizedMouse))
    {
        hoveredTarget.type = SettingsHoverTargetType::ApplyButton;
    }
    else
    {
        for (const SettingsCategoryEntryViewModel& category : page.categories)
        {
            if (overlayui::contains(category.bounds, normalizedMouse))
            {
                hoveredTarget.type = SettingsHoverTargetType::Category;
                hoveredTarget.category = category.category;
                break;
            }
        }

        if (hoveredTarget.type == SettingsHoverTargetType::None)
        {
            for (const SettingsPanelItemViewModel& item : page.items)
            {
                if (!overlayui::contains(page.chrome.contentViewportBounds,
                                         Vec2{normalizedMouse.x, normalizedMouse.y}) ||
                    !overlayui::contains(item.bounds, normalizedMouse))
                {
                    continue;
                }

                if (item.kind == SettingsPanelItemKind::Segmented)
                {
                    for (const SettingsSegmentOptionViewModel& option : item.segmentOptions)
                    {
                        if (overlayui::contains(option.bounds, normalizedMouse))
                        {
                            hoveredTarget = option.hoverTarget;
                            break;
                        }
                    }
                    if (hoveredTarget.type != SettingsHoverTargetType::None)
                    {
                        break;
                    }
                }

                if (item.hoverTarget.type != SettingsHoverTargetType::None &&
                    overlayui::contains(item.interactiveBounds, normalizedMouse))
                {
                    hoveredTarget = item.hoverTarget;
                    break;
                }
            }
        }
    }

    if (m_hoverTarget != hoveredTarget)
    {
        m_hoverTarget = hoveredTarget;
        markDirty(dirtyRegionForHoverTarget(previousHoverTarget) |
                  dirtyRegionForHoverTarget(hoveredTarget));
    }
}

void SettingsOverlay::updateSliderFromMouse(SettingsHoverTargetType sliderType,
                                            const Vec2& normalizedMouse)
{
    const SettingsPageModel page = buildSettingsPageModel(viewModel());
    overlayui::Rect sliderRect{};
    for (const SettingsPanelItemViewModel& item : page.items)
    {
        if (item.hoverTarget.type == sliderType)
        {
            sliderRect = item.interactiveBounds;
            break;
        }
    }

    const float sliderWidth = sliderRect.right - sliderRect.left;
    if (sliderWidth <= 0.0f)
    {
        return;
    }

    const float t = overlayui::normalizedT(normalizedMouse.x, sliderRect.left, sliderRect.right);
    switch (sliderType)
    {
    case SettingsHoverTargetType::ResolutionSlider:
    {
        const int newIndex = static_cast<int>(
            std::round(t * static_cast<float>(static_cast<int>(kResolutionOptions.size()) - 1)));
        if (newIndex != m_resolutionIndex)
        {
            m_resolutionIndex = newIndex;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    }
    case SettingsHoverTargetType::MasterVolumeSlider:
        if (std::fabs(m_masterVolume - t) > 0.001f)
        {
            m_masterVolume = t;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    case SettingsHoverTargetType::MusicVolumeSlider:
        if (std::fabs(m_musicVolume - t) > 0.001f)
        {
            m_musicVolume = t;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    case SettingsHoverTargetType::MouseSensitivitySlider:
        if (std::fabs(m_mouseSensitivity - t) > 0.001f)
        {
            m_mouseSensitivity = t;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    case SettingsHoverTargetType::TypingSpeedSlider:
        if (std::fabs(m_typingSpeed - t) > 0.001f)
        {
            m_typingSpeed = t;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    default:
        break;
    }
}

void SettingsOverlay::applyPendingSettings(Application& application)
{
    const ResolutionOption& resolution =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)];
    application.setWindowResolution(resolution.width, resolution.height);
    application.setWindowMode(m_windowMode);
    application.setVSyncEnabled(m_vsyncEnabled);
    application.setMasterVolume(m_masterVolume);
    application.setMusicVolume(m_musicVolume);
    application.setMouseSensitivity(
        SettingsValueMapping::denormalizeMouseSensitivity(m_mouseSensitivity));
    application.setVnTypingCharactersPerSecond(
        SettingsValueMapping::denormalizeTypingCharactersPerSecond(m_typingSpeed));
    application.setVnAutoAdvanceEnabled(m_autoAdvanceEnabled);
    syncFromApplication(application);
    markDirty(SettingsOverlayDirtyRegion::Content);
}

bool SettingsOverlay::hasPendingChanges() const noexcept
{
    const ResolutionOption& resolution =
        kResolutionOptions[static_cast<std::size_t>(m_resolutionIndex)];
    return resolution.width != m_appliedSettings.width ||
           resolution.height != m_appliedSettings.height ||
           m_windowMode != m_appliedSettings.windowMode ||
           m_vsyncEnabled != m_appliedSettings.vSyncEnabled ||
           std::fabs(m_masterVolume - m_appliedMasterVolume) > 0.001f ||
           std::fabs(m_musicVolume - m_appliedMusicVolume) > 0.001f ||
           std::fabs(SettingsValueMapping::denormalizeMouseSensitivity(m_mouseSensitivity) -
                     m_appliedMouseSensitivity) > 0.001f ||
           std::fabs(SettingsValueMapping::denormalizeTypingCharactersPerSecond(m_typingSpeed) -
                     m_appliedTypingCharactersPerSecond) > 0.001f ||
           m_autoAdvanceEnabled != m_appliedAutoAdvanceEnabled;
}

void SettingsOverlay::markDirty(SettingsOverlayDirtyRegion dirtyRegion) noexcept
{
    m_dirtyRegions |= dirtyRegion;
}
} // namespace engine
