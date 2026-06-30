#include "SettingsOverlay.h"

#include "SettingsPage.h"

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
    m_draggingScrollThumb = false;
    m_scrollThumbGrabOffset = 0.0f;
    m_contentScrollOffset = 0.0f;
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
        if (inputState.mouseDown && m_draggingScrollThumb)
        {
            updateScrollFromMouse(normalizedMouse, false);
            const SettingsHoverTarget draggingHoverTarget{SettingsHoverTargetType::ScrollThumb,
                                                          m_activeCategory};
            if (m_hoverTarget != draggingHoverTarget)
            {
                m_hoverTarget = draggingHoverTarget;
                markDirty(SettingsOverlayDirtyRegion::Content);
            }
        }
        else if (inputState.mouseDown && m_draggingSlider != SettingsHoverTargetType::None)
        {
            updateSliderFromMouse(m_draggingSlider, normalizedMouse, application);
            const SettingsHoverTarget draggingHoverTarget{m_draggingSlider, m_activeCategory};
            if (m_hoverTarget != draggingHoverTarget)
            {
                m_hoverTarget = draggingHoverTarget;
                markDirty(SettingsOverlayDirtyRegion::Content);
            }
        }

        if (std::fabs(inputState.mouseScrollDelta) > 0.001f)
        {
            scrollContent(inputState.mouseScrollDelta, normalizedMouse);
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
        m_draggingScrollThumb = false;
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
            m_draggingScrollThumb = false;
            m_scrollThumbGrabOffset = 0.0f;
            m_contentScrollOffset = 0.0f;
            markDirty(SettingsOverlayDirtyRegion::Base | SettingsOverlayDirtyRegion::Content);
        }
        return Result::None;
    case SettingsHoverTargetType::ResolutionSlider:
    case SettingsHoverTargetType::MasterVolumeSlider:
    case SettingsHoverTargetType::MusicVolumeSlider:
    case SettingsHoverTargetType::UiVolumeSlider:
    case SettingsHoverTargetType::VnVolumeSlider:
    case SettingsHoverTargetType::AmbientVolumeSlider:
    case SettingsHoverTargetType::SfxVolumeSlider:
    case SettingsHoverTargetType::MouseSensitivitySlider:
    case SettingsHoverTargetType::TypingSpeedSlider:
        m_draggingSlider = m_hoverTarget.type;
        if (hasMouse)
        {
            updateSliderFromMouse(m_hoverTarget.type, normalizedMouse, application);
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
    case SettingsHoverTargetType::ScrollTrack:
    case SettingsHoverTargetType::ScrollThumb:
        m_draggingSlider = SettingsHoverTargetType::None;
        m_draggingScrollThumb = true;
        updateScrollFromMouse(normalizedMouse,
                              m_hoverTarget.type == SettingsHoverTargetType::ScrollTrack);
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
    viewModel.uiVolume = m_uiVolume;
    viewModel.vnVolume = m_vnVolume;
    viewModel.ambientVolume = m_ambientVolume;
    viewModel.sfxVolume = m_sfxVolume;
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
    m_uiVolume = application.audioSettings().uiVolume;
    m_vnVolume = application.audioSettings().vnVolume;
    m_ambientVolume = application.audioSettings().ambientVolume;
    m_sfxVolume = application.audioSettings().sfxVolume;
    m_mouseSensitivity = SettingsValueMapping::normalizeMouseSensitivity(
        application.inputSettings().mouseSensitivity);
    m_typingSpeed = SettingsValueMapping::normalizeTypingCharactersPerSecond(
        application.vnSettings().typingCharactersPerSecond);
    m_autoAdvanceEnabled = application.vnSettings().autoAdvanceEnabled;
    m_appliedMasterVolume = application.audioSettings().masterVolume;
    m_appliedMusicVolume = application.audioSettings().musicVolume;
    m_appliedUiVolume = application.audioSettings().uiVolume;
    m_appliedVnVolume = application.audioSettings().vnVolume;
    m_appliedAmbientVolume = application.audioSettings().ambientVolume;
    m_appliedSfxVolume = application.audioSettings().sfxVolume;
    m_appliedMouseSensitivity = application.inputSettings().mouseSensitivity;
    m_appliedTypingCharactersPerSecond = application.vnSettings().typingCharactersPerSecond;
    m_appliedAutoAdvanceEnabled = application.vnSettings().autoAdvanceEnabled;
}

void SettingsOverlay::scrollContent(const float scrollDelta, const Vec2& normalizedMouse) noexcept
{
    const SettingsPageModel page = buildSettingsPageModel(viewModel());
    if (!overlayui::contains(page.chrome.contentViewportBounds, normalizedMouse) &&
        !overlayui::contains(page.chrome.scrollTrackBounds, normalizedMouse))
    {
        return;
    }

    const float viewportHeight =
        page.chrome.contentViewportBounds.bottom - page.chrome.contentViewportBounds.top;
    const float scrollRange = std::max(page.contentHeight - viewportHeight, 0.0f);
    if (scrollRange <= 0.0f)
    {
        return;
    }

    constexpr float kScrollStepPixels = 96.0f;
    const float previousOffset = m_contentScrollOffset;
    m_contentScrollOffset =
        std::clamp(m_contentScrollOffset - scrollDelta * kScrollStepPixels, 0.0f, scrollRange);
    if (std::fabs(m_contentScrollOffset - previousOffset) > 0.001f)
    {
        markDirty(SettingsOverlayDirtyRegion::Content);
    }
}

void SettingsOverlay::updateScrollFromMouse(const Vec2& normalizedMouse,
                                            const bool centerOnCursor) noexcept
{
    const SettingsPageModel page = buildSettingsPageModel(viewModel());
    const float viewportHeight =
        page.chrome.contentViewportBounds.bottom - page.chrome.contentViewportBounds.top;
    const float scrollRange = std::max(page.contentHeight - viewportHeight, 0.0f);
    if (scrollRange <= 0.0f)
    {
        m_contentScrollOffset = 0.0f;
        m_draggingScrollThumb = false;
        m_scrollThumbGrabOffset = 0.0f;
        return;
    }

    const float trackHeight =
        page.chrome.scrollTrackBounds.bottom - page.chrome.scrollTrackBounds.top;
    const float thumbHeight =
        page.chrome.scrollThumbBounds.bottom - page.chrome.scrollThumbBounds.top;
    const float thumbTravel = std::max(trackHeight - thumbHeight, 0.0f);
    if (thumbTravel <= 0.0f)
    {
        return;
    }

    if (centerOnCursor)
    {
        m_scrollThumbGrabOffset = thumbHeight * 0.5f;
    }
    else
    {
        m_scrollThumbGrabOffset =
            std::clamp(m_scrollThumbGrabOffset, 0.0f, std::max(thumbHeight, 0.0f));
    }

    const float thumbTop =
        std::clamp(normalizedMouse.y - m_scrollThumbGrabOffset, page.chrome.scrollTrackBounds.top,
                   page.chrome.scrollTrackBounds.bottom - thumbHeight);
    const float scrollT = (thumbTop - page.chrome.scrollTrackBounds.top) / thumbTravel;
    const float previousOffset = m_contentScrollOffset;
    m_contentScrollOffset = std::clamp(scrollT * scrollRange, 0.0f, scrollRange);
    if (std::fabs(m_contentScrollOffset - previousOffset) > 0.001f)
    {
        markDirty(SettingsOverlayDirtyRegion::Content);
    }
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
            if (overlayui::contains(page.chrome.scrollThumbBounds, normalizedMouse))
            {
                hoveredTarget.type = SettingsHoverTargetType::ScrollThumb;
                m_scrollThumbGrabOffset = std::clamp(
                    normalizedMouse.y - page.chrome.scrollThumbBounds.top, 0.0f,
                    page.chrome.scrollThumbBounds.bottom - page.chrome.scrollThumbBounds.top);
            }
            else if (overlayui::contains(page.chrome.scrollTrackBounds, normalizedMouse))
            {
                hoveredTarget.type = SettingsHoverTargetType::ScrollTrack;
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
                                            const Vec2& normalizedMouse, Application& application)
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
            application.setMasterVolume(m_masterVolume);
            m_appliedMasterVolume = application.audioSettings().masterVolume;
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    case SettingsHoverTargetType::MusicVolumeSlider:
    case SettingsHoverTargetType::UiVolumeSlider:
    case SettingsHoverTargetType::VnVolumeSlider:
    case SettingsHoverTargetType::AmbientVolumeSlider:
    case SettingsHoverTargetType::SfxVolumeSlider:
    {
        float* currentValue = nullptr;
        float* appliedValue = nullptr;
        AudioCategory category = AudioCategory::Music;

        switch (sliderType)
        {
        case SettingsHoverTargetType::MusicVolumeSlider:
            currentValue = &m_musicVolume;
            appliedValue = &m_appliedMusicVolume;
            category = AudioCategory::Music;
            break;
        case SettingsHoverTargetType::UiVolumeSlider:
            currentValue = &m_uiVolume;
            appliedValue = &m_appliedUiVolume;
            category = AudioCategory::UI;
            break;
        case SettingsHoverTargetType::VnVolumeSlider:
            currentValue = &m_vnVolume;
            appliedValue = &m_appliedVnVolume;
            category = AudioCategory::VN;
            break;
        case SettingsHoverTargetType::AmbientVolumeSlider:
            currentValue = &m_ambientVolume;
            appliedValue = &m_appliedAmbientVolume;
            category = AudioCategory::Ambient;
            break;
        case SettingsHoverTargetType::SfxVolumeSlider:
            currentValue = &m_sfxVolume;
            appliedValue = &m_appliedSfxVolume;
            category = AudioCategory::SFX;
            break;
        default:
            break;
        }

        if (currentValue != nullptr && appliedValue != nullptr &&
            std::fabs(*currentValue - t) > 0.001f)
            m_contentScrollOffset = 0.0f;
        {
            *currentValue = t;
            application.setAudioCategoryVolume(category, *currentValue);
            *appliedValue = application.audioCategoryVolume(category);
            markDirty(SettingsOverlayDirtyRegion::Content);
        }
        break;
    }
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
    application.setAudioCategoryVolume(AudioCategory::Music, m_musicVolume);
    application.setAudioCategoryVolume(AudioCategory::UI, m_uiVolume);
    application.setAudioCategoryVolume(AudioCategory::VN, m_vnVolume);
    application.setAudioCategoryVolume(AudioCategory::Ambient, m_ambientVolume);
    application.setAudioCategoryVolume(AudioCategory::SFX, m_sfxVolume);
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
           std::fabs(m_uiVolume - m_appliedUiVolume) > 0.001f ||
           std::fabs(m_vnVolume - m_appliedVnVolume) > 0.001f ||
           std::fabs(m_ambientVolume - m_appliedAmbientVolume) > 0.001f ||
           std::fabs(m_sfxVolume - m_appliedSfxVolume) > 0.001f ||
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
